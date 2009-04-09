DIRS = vicam dabusb ttusb-budget cpia2

TARGETS = ihex2fw vicam/firmware.fw dabusb/firmware.fw dabusb/bitstream.bin ttusb-budget/dspbootcode.bin cpia2/stv0672_vp4.bin

####

default: $(TARGETS)

clean:
	-rm -f $(TARGETS)

distclean: clean
	-for i in $(DIRS); do if [ -d $$i ]; then rmdir $$i; fi; done

install: default
	-for i in $(DIRS); do if [ ! -d /lib/firmware/$$i ]; then mkdir -p /lib/firmware/$$i; fi; done
	-for i in $(TARGETS); do cp $$i /lib/firmware/$$i; done


######

mkdir:
	-for i in $(DIRS); do if [ ! -d $$i ]; then mkdir -p $$i; fi; done


ihex2fw: ../../linux/firmware/ihex2fw.c
	gcc -Wall -o $@ $<

vicam/firmware.fw: ../../linux/firmware/vicam/firmware.H16 mkdir
	./ihex2fw -w $< $@

dabusb/firmware.fw: ../../linux/firmware/dabusb/firmware.HEX mkdir
	./ihex2fw $< $@

ttusb-budget/dspbootcode.bin: ../../linux/firmware/ttusb-budget/dspbootcode.bin.ihex mkdir
	objcopy -Iihex -Obinary $< $@


dabusb/bitstream.bin: ../../linux/firmware/dabusb/bitstream.bin.ihex mkdir
	objcopy -Iihex -Obinary $< $@

cpia2/stv0672_vp4.bin: ../../linux/firmware/cpia2/stv0672_vp4.bin.ihex mkdir
	objcopy -Iihex -Obinary $< $@

