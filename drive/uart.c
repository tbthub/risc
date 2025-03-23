#include "core/signal.h"
#include "defs.h"
#include "lib/spinlock.h"
#include "mm/memlayout.h"
#include "riscv.h"
#include "std/stddef.h"
#include "std/stdio.h"

// the UART control registers are memory-mapped
// at address UART0. this macro returns the
// address of one of the registers.
#define Reg(reg) ((volatile unsigned char *)(UART0 + (reg)))

#define RHR 0  // receive holding register (for input bytes)
#define THR 0  // transmit holding register (for output bytes)
#define IER 1  // interrupt enable register
#define IER_RX_ENABLE (1 << 0)
#define IER_TX_ENABLE (1 << 1)
#define FCR 2  // FIFO control register
#define FCR_FIFO_ENABLE (1 << 0)
#define FCR_FIFO_CLEAR (3 << 1)  // clear the content of the two FIFOs
#define ISR 2                    // interrupt status register
#define LCR 3                    // line control register
#define LCR_EIGHT_BITS (3 << 0)
#define LCR_BAUD_LATCH (1 << 7)  // special mode to set baud rate
#define LSR 5                    // line status register
#define LSR_RX_READY (1 << 0)    // input is waiting to be read from RHR
#define LSR_TX_IDLE (1 << 5)     // THR can accept another character to send

#define ReadReg(reg) (*(Reg(reg)))
#define WriteReg(reg, v) (*(Reg(reg)) = (v))

// the transmit output buffer.
spinlock_t uart_tx_lock;
#define UART_TX_BUF_SIZE 32
char uart_tx_buf[UART_TX_BUF_SIZE];
uint64 uart_tx_w;  // write next to uart_tx_buf[uart_tx_w % UART_TX_BUF_SIZE]
uint64 uart_tx_r;  // read next from uart_tx_buf[uart_tx_r % UART_TX_BUF_SIZE]

void uart_init()
{
    // 关中断
    WriteReg(IER, 0x00);

    // special mode to set baud rate.
    WriteReg(LCR, LCR_BAUD_LATCH);

    // LSB for baud rate of 38.4K.
    // 设置波特率为 38_400
    WriteReg(0, 0x03);

    // MSB for baud rate of 38.4K.
    WriteReg(1, 0x00);

    // leave set-baud mode,
    // and set word length to 8 bits, no parity.
    // 退出设定波特率模式并设置数据格式
    WriteReg(LCR, LCR_EIGHT_BITS);

    // 重置和启用 FIFO
    WriteReg(FCR, FCR_FIFO_ENABLE | FCR_FIFO_CLEAR);

    // 启用发送和接收中断
    WriteReg(IER, IER_RX_ENABLE); 
    // WriteReg(IER, IER_TX_ENABLE | IER_RX_ENABLE);

    spin_init(&uart_tx_lock, "uart_tx");
}

// 进入循环，检查发送缓冲区是否为空。如果为空，返回函数，不做任何发送操作。
// 检查 UART 是否忙碌。如果发送保持寄存器满了，函数返回，等待 UART
// 准备好接受新数据。 如果有数据待发送且 UART
// 空闲，从缓冲区中取出一个字符，将读指针加 1。 发送这个字符到 UART，通知 UART
// 开始传输，并唤醒等待的进程。 循环重复上述步骤，直到发送缓冲区为空或 UART
// 忙碌。

// 缓冲区内容输出
// 将会唤醒那些因为缓冲区满而堵塞的线程
void uart_start()
{
    while (1) {
        if (uart_tx_w == uart_tx_r) {
            // 缓冲区空
            ReadReg(ISR);
            return;
        }

        if ((ReadReg(LSR) & LSR_TX_IDLE) == 0) {
            // the UART transmit holding register is full,
            // so we cannot give it another byte.
            // it will interrupt when it's ready for a new byte.
            return;
        }

        int c = uart_tx_buf[uart_tx_r++ % UART_TX_BUF_SIZE];

        // 唤醒那些需要写缓冲区的线程（假如有的话）
        // wakeup(&uart_tx_r);
        // 这里待完成

        // 将读取的字符 c 写入 UART 的发送保持寄存器（THR），触发 UART
        // 发送该字符。
        WriteReg(THR, c);
    }
}

// 往缓冲区输入一个字符
// 需要申请锁
void uart_putc(int c)
{
    spin_lock(&uart_tx_lock);

    while (uart_tx_w == uart_tx_r + UART_TX_BUF_SIZE) {
        // 缓冲区已满
        // 待修改，加入睡眠
        return;
    }
    uart_tx_buf[uart_tx_w++ % UART_TX_BUF_SIZE] = c;

    uart_start();
    spin_unlock(&uart_tx_lock);
}

// 不使用缓冲区，直接写（插队）
// 适用于 kernel print
// 需要自旋等待输出缓冲区为空
void uart_putc_sync(int c)
{
    push_off();

    // wait for Transmit Holding Empty to be set in LSR.
    while ((ReadReg(LSR) & LSR_TX_IDLE) == 0)
        ;
        
    WriteReg(THR, c);

    pop_off();
}

int uart_getc()
{
    if (ReadReg(LSR) & 0x01) {
        // input data is ready.
        return ReadReg(RHR);
    }
    else
        return ERR;
}

void uartintr()
{
    int ch = uart_getc();
    if (ch == ERR)
        return;

    switch (ch) {
    case '\r':  // 按下回车，只是\r，但是我们为了习惯，额外加上一个换行符
        uart_putc_sync('\n');
        break;

    case 127:  // 退格
        uart_putc_sync('\b');
        uart_putc_sync(' ');
        uart_putc_sync('\b');
        break;

    default:
        // printk("(%d): ",ch);
        uart_putc_sync(ch);
        // printk("\n");
        break;
    }
    if (ch < 32) {
		printk("(%d) sig\n",ch);
        send_sig(ch, 0);
    }
}
