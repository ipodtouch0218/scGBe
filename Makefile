# Compiler
CC := g++

# Directories
SRCDIR := src
INCDIR := include
MINGW_INCDIR := C:/msys-w64/ucrt64/lib/gcc/x86_64-w64-mingw32/13.2.0/include
BUILDDIR := build

# Files to compile
SRCEXT := cpp
HEADERS := $(wildcard $(INCDIR)/*)
SOURCES := $(wildcard $(SRCDIR)/*.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))

# Flags
CFLAGS := -I$(INCDIR) -I$(MINGW_INCDIR) -lstdc++ -IC:/msys-w64

# Output binary
TARGET := myprogram

# Main target
all: $(TARGET)

# Compile
$(TARGET): $(OBJECTS)
	@echo " Linking..."
	$(CC) $^ -o $(TARGET)

# Compile source files to object files
$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT) $(HEADERS)
	@mkdir -p $(BUILDDIR)
	@echo " Compiling $<..."
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean
clean:
	@echo " Cleaning...";
	$(RM) -r $(BUILDDIR) $(TARGET)

.PHONY: clean
