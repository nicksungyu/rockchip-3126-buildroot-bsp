BTE_MENU_SITE_METHOD = local
BTE_MENU_SITE = $(TOPDIR)/package/bte_menu/src
BTE_MENU_DEPENDENCIES = host-pkgconf sdl2 sdl2_image sdl2_ttf sdl2_mixer libusb alsa-lib alsa-utils wireless_tools iw
BTE_MENU_BUILD_OPTS = -I$(STAGING_DIR)/usr/include -L$(STAGING_DIR)/usr/lib

BTE_MENU_MAKE = $(MAKE) BOARD=$(BR2_ARCH)

BTE_MENU_MAKE_OPTS = CC="$(TARGET_CC) $(BTE_MENU_BUILD_OPTS)" \
		    CXX="$(TARGET_CXX) $(BTE_MENU_BUILD_OPTS)"	

define BTE_MENU_CONFIGURE_CMDS
	# Do NOTHING
endef

define BTE_MENU_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(BTE_MENU_MAKE) -C  $(@D) $(BTE_MENU_MAKE_OPTS)
endef

define BTE_MENU_INSTALL_TARGET_CMDS
        $(INSTALL) -D -m 0755 $(@D)/menu $(TARGET_DIR)/usr/bin
        $(INSTALL) -D -m 0755 $(@D)/fingerprint $(TARGET_DIR)/sbin
        $(INSTALL) -D -m 0755 $(@D)/showImage $(TARGET_DIR)/usr/bin
        $(TARGET_STRIP) $(TARGET_DIR)/usr/bin/menu
        $(TARGET_STRIP) $(TARGET_DIR)/sbin/fingerprint
endef

$(eval $(autotools-package))

