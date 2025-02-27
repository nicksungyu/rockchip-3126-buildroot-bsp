################################################################################
#
# QFacialGate
#
################################################################################

QFACIALGATE_VERSION = 1.0
QFACIALGATE_SITE = $(TOPDIR)/../app/QFacialGate
QFACIALGATE_SITE_METHOD = local

QFACIALGATE_DEPENDENCIES = rkfacial qt5base

ifeq ($(BR2_PACKAGE_QT5BASE_LINUXFB_RGB565),y)
QFACIALGATE_CONFIGURE_OPTS += QMAKE_CXXFLAGS+=-DQT_FB_DRM_RGB565
else ifeq ($(BR2_PACKAGE_QT5BASE_LINUXFB_RGB32),y)
QFACIALGATE_CONFIGURE_OPTS += QMAKE_CXXFLAGS+=-DQT_FB_DRM_RGB32
else ifeq ($(BR2_PACKAGE_QT5BASE_LINUXFB_ARGB32),y)
QFACIALGATE_CONFIGURE_OPTS += QMAKE_CXXFLAGS+=-DQT_FB_DRM_ARGB32
endif

ifeq ($(BR2_PACKAGE_RV1126_RV1109),y)
QFACIALGATE_CONFIGURE_OPTS += QMAKE_CXXFLAGS+=-DTWO_PLANE
else
QFACIALGATE_CONFIGURE_OPTS += QMAKE_CXXFLAGS+=-DONE_PLANE
endif

ifeq ($(BR2_PACKAGE_QFACIALGATE_TEST),y)
QFACIALGATE_CONFIGURE_OPTS += QMAKE_CXXFLAGS+=-DBUILD_TEST
endif

define QFACIALGATE_CONFIGURE_CMDS
	cd $(@D); $(TARGET_MAKE_ENV) $(HOST_DIR)/bin/qmake $(QFACIALGATE_CONFIGURE_OPTS)
endef

define QFACIALGATE_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D)
endef

define QFACIALGATE_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/QFacialGate	$(TARGET_DIR)/usr/bin/QFacialGate
	if [ x${BR2_PACKAGE_RK1806} != x ]; then \
		$(INSTALL) -D -m 0755 $(@D)/S06_QFacialGate $(TARGET_DIR)/etc/init.d/; \
	fi
	if [ x${BR2_PACKAGE_RK1808} != x ]; then \
		$(INSTALL) -D -m 0755 $(@D)/S06_QFacialGate $(TARGET_DIR)/etc/init.d/; \
	fi
endef

$(eval $(generic-package))
