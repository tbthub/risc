
$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $(LIB) -o $@ $(OBJ)

