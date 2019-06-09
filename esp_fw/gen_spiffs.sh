mkspiffs -c storage -b 4096 -p 256 -s 0x100000 build/spiffs.bin
python ${IDF_PATH}/components/esptool_py/esptool/esptool.py --chip esp32 -b 460800 write_flash --flash_mode dio --flash_freq 40m --flash_size 4MB 0x210000 build/spiffs.bin