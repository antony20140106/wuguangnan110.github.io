#
# Copyright (C) 2008-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 1484 2014-04-03 09:42:01Z danfe $
#

include $(TOPDIR)/rules.mk

MODULE_NAME:=mp2723
PKG_NAME:=sl8521e_$(MODULE_NAME)
PKG_RELEASE:=1

define KernelPackage/$(PKG_NAME)
  TITLE:=$(PKG_NAME)
  DEPENDS:=
  FILES:=$(PKG_BUILD_DIR)/$(MODULE_NAME).ko
  KCONFIG:=
  SUBMENU:=sl8521e
  SUBMENUDEP:=TARGET_sl8521e
endef

define KernelPackage/$(PKG_NAME)/description
  Kernel module to register $(PKG_NAME) device
endef

include $(INCLUDE_DIR)/kmod-prolin.mk

$(eval $(call KernelPackage,$(PKG_NAME)))
