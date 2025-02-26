BTE_OTA_SITE_METHOD = local
BTE_OTA_SITE = $(TOPDIR)/package/bte_ota/src
BTE_OTA_DEPENDENCIES = host-pkgconf sdl2 sdl2_image sdl2_ttf sdl2_mixer alsa-lib alsa-utils
BTE_OTA_BUILD_OPTS =

BTE_OTA_MAKE = $(MAKE) BOARD=$(BR2_ARCH)

BTE_OTA_MAKE_OPTS = CC="$(TARGET_CC) $(BTE_OTA_BUILD_OPTS)" \
		    CXX="$(TARGET_CXX) $(BTE_OTA_BUILD_OPTS)"	

define BTE_OTA_CONFIGURE_CMDS
	# Do NOTHING
endef

define BTE_OTA_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(BTE_OTA_MAKE) -C  $(@D) $(BTE_OTA_MAKE_OPTS)
endef

define BTE_OTA_INSTALL_TARGET_CMDS
        $(INSTALL) -D -m 0755 $(@D)/ota $(TARGET_DIR)/usr/bin
        $(TARGET_STRIP) $(TARGET_DIR)/usr/bin/ota
        cp $(@D)/ota_key.priv $(TARGET_DIR)/root
        cp $(@D)/fw_update.sh $(TARGET_DIR)/sbin
        cp $(@D)/DroidSerif-Bold.ttf $(TARGET_DIR)/root
endef

$(eval $(autotools-package))

