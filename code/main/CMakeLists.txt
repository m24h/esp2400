idf_component_register(
	       SRC_DIRS "."
           INCLUDE_DIRS "."
           PRIV_REQUIRES 
           	    "CS1237" "ST7789" "ios" "tools"
           		"driver" "mdns" "esp_adc_cal" "fatfs" "wear_levelling"
           		"console" "nvs_flash" "esp_wifi" "lwip" "esp_http_server"
 	       EMBED_FILES 
		       	"message.fon" "message.idx" "hz.fon" "hz.idx"
		       	"ivc.fon" "ivc.idx" "ivc2.fon" "ivc2.idx"
		       	"ivs.fon" "ivs.idx" "ivs2.fon" "ivs2.idx" 
		       	"ivs3.fon" "ivs3.idx"  "ivs4.fon" "ivs4.idx"
		       	"menu.fon" "menu.idx" "menuhz.fon" "menuhz.idx"
		       	"listbtm.pic" "listtop.pic"
)

#notice :
#  fatfs_create_spiflash_image has some bug in verson before 2021-12-9
#  need to change file components/fatfs/wl_fatfsgen.py, at line 168, add "wb" method to call "open" function 
#   as "with open(output_path, 'wb') as output:"
#  and change file components/fatfs/fatfsgen_utils/fs_object.py , at line 50, remove "encode()" method,
#   as "content_as_list = content_part"
# WHY does a partition generator want to encode my raw file for some character?
# and also, fatfs_create_spiflash_image will pollutes the partition just next the generated one,
#  I need to reserve a gap between them
# ....And finally, I tried to use fatfs_create_rawflash_image for read-only resources
fatfs_create_rawflash_image (resource resource FLASH_IN_PROJECT)
#fatfs_create_spiflash_image (resource resource FLASH_IN_PROJECT)
