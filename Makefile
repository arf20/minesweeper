CC          := gcc
LD			:= ld

TARGET      := arfminesweeper

SRCDIR      := main_src
BUILDDIR    := obj
TARGETDIR   := bin
RESDIR      := assets
SRCEXT      := c
DEPEXT      := d
OBJEXT      := o

CFLAGS      := -Wall -pedantic -g
LDFLAGS     := 

SOURCES     := $(SRCDIR)/main.c
OBJECTS     := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.$(OBJEXT)))

# detect libs


#Defauilt Make
all: resources $(TARGET)

#Remake
remake: clean all

#Copy Resources from Resources Directory to Target Directory
resources: directories
    @cp $(RESDIR)/* $(TARGETDIR)/

#Make the Directories
directories:
    @mkdir -p $(TARGETDIR)
    @mkdir -p $(BUILDDIR)

#Clean only Objecst
clean:
    @$(RM) -rf $(BUILDDIR)
	@$(RM) -rf $(TARGETDIR)    

#Pull in dependency info for *existing* .o files
-include $(OBJECTS:.$(OBJEXT)=.$(DEPEXT))

#Link
$(TARGET): $(OBJECTS)
    $(CC) -o $(TARGETDIR)/$(TARGET) $^ $(LIB)

#Compile
$(BUILDDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT)
    @mkdir -p $(dir $@)
    $(CC) $(CFLAGS) $(INC) -c -o $@ $<
    @$(CC) $(CFLAGS) $(INCDEP) -MM $(SRCDIR)/$*.$(SRCEXT) > $(BUILDDIR)/$*.$(DEPEXT)
    @cp -f $(BUILDDIR)/$*.$(DEPEXT) $(BUILDDIR)/$*.$(DEPEXT).tmp
    @sed -e 's|.*:|$(BUILDDIR)/$*.$(OBJEXT):|' < $(BUILDDIR)/$*.$(DEPEXT).tmp > $(BUILDDIR)/$*.$(DEPEXT)
    @sed -e 's/.*://' -e 's/\\$$//' < $(BUILDDIR)/$*.$(DEPEXT).tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $(BUILDDIR)/$*.$(DEPEXT)
    @rm -f $(BUILDDIR)/$*.$(DEPEXT).tmp

#Non-File Targets
.PHONY: all remake clean cleaner resources

