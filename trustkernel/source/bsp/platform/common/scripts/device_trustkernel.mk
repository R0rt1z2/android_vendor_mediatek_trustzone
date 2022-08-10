ifeq ($(strip $(TRUSTKERNEL_TEE_SUPPORT)), yes)

	#ADDITIONAL_BUILD_PROPERTIES += ro.mtk.trustkernel_tee_support=1

	EXECUTABLE_ARCH := $(if $(filter yes, $(MTK_K64_SUPPORT)),arm64-v8a,armeabi-v7a)

	PRODUCT_PACKAGES += libteec

	# for Production Line
	PRODUCT_PACKAGES += libpl
	PRODUCT_PACKAGES += libkphproxy
	PRODUCT_PACKAGES += kmsetkey.trustkernel

	# Android TUI service package
	PRODUCT_PACKAGES += TUIService

	# Android build-in services
	PRODUCT_PACKAGES += gatekeeper.trustkernel
	PRODUCT_PACKAGES += keystore.trustkernel

	# Android configuration files
	PRODUCT_COPY_FILES += vendor/mediatek/proprietary/trustzone/trustkernel/source/bsp/platform/common/scripts/trustkernel.rc:/vendor/etc/init/trustkernel.rc

	# tee clients and libs
	PRODUCT_COPY_FILES += vendor/mediatek/proprietary/trustzone/trustkernel/source/client/libs/$(EXECUTABLE_ARCH)/teed:vendor/bin/teed

	PRODUCT_COPY_FILES += vendor/mediatek/proprietary/trustzone/trustkernel/source/client/libs/arm64-v8a/libteec.so:vendor/lib64/libteec.so
	PRODUCT_COPY_FILES += vendor/mediatek/proprietary/trustzone/trustkernel/source/client/libs/armeabi-v7a/libteec.so:vendor/lib/libteec.so

	# CA/libteeclientjni
	# PRODUCT_COPY_FILES += vendor/mediatek/proprietary/trustzone/trustkernel/source/ca/libs/arm64-v8a/libteeclientjni.so:vendor/lib64/libteeclientjni.so
	# PRODUCT_COPY_FILES += vendor/mediatek/proprietary/trustzone/trustkernel/source/ca/libs/armeabi-v7a/libteeclientjni.so:vendor/lib/libteeclientjni.so

	# IFAA libs
	#PRODUCT_COPY_FILES += vendor/mediatek/proprietary/trustzone/trustkernel/source/ca/libs/arm64-v8a/libifaabmtjni.so:system/lib64/libifaabmtjni.so
	#PRODUCT_COPY_FILES += vendor/mediatek/proprietary/trustzone/trustkernel/source/ca/libs/armeabi-v7a/libifaabmtjni.so:system/lib/libifaabmtjni.so
	#PRODUCT_COPY_FILES += vendor/mediatek/proprietary/trustzone/trustkernel/source/ca/libs/arm64-v8a/ifaa-bmt:system/bin/ifaa-bmt

	# KPH ca
	PRODUCT_COPY_FILES += vendor/mediatek/proprietary/trustzone/trustkernel/source/ca/libs/$(EXECUTABLE_ARCH)/kph:vendor/bin/kph
	PRODUCT_COPY_FILES += vendor/mediatek/proprietary/trustzone/trustkernel/source/ca/libs/$(EXECUTABLE_ARCH)/pld:vendor/bin/pld

	# TAs
	# keymaster_v1
	#PRODUCT_COPY_FILES += vendor/mediatek/proprietary/trustzone/trustkernel/source/ta/2ea702fa-17bc-4752-b3adb2871a772347.ta:vendor/app/t6/2ea702fa-17bc-4752-b3adb2871a772347.ta

	# gatekeeper
	PRODUCT_COPY_FILES += vendor/mediatek/proprietary/trustzone/trustkernel/source/ta/02662e8e-e126-11e5-b86d9a79f06e9478.ta:vendor/app/t6/02662e8e-e126-11e5-b86d9a79f06e9478.ta
	# keymaster_v2
	PRODUCT_COPY_FILES += vendor/mediatek/proprietary/trustzone/trustkernel/source/ta/9ef77781-7bd5-4e39-965f20f6f211f46b.ta:vendor/app/t6/9ef77781-7bd5-4e39-965f20f6f211f46b.ta
	# ifaa_v2
	#PRODUCT_COPY_FILES += vendor/mediatek/proprietary/trustzone/trustkernel/source/ta/8b1e0e41-2636-11e1-ad9e0002a5d5c51b.ta:vendor/app/t6/8b1e0e41-2636-11e1-ad9e0002a5d5c51b.ta
	# kph
	PRODUCT_COPY_FILES += vendor/mediatek/proprietary/trustzone/trustkernel/source/ta/b46325e6-5c90-8252-2eada8e32e5180d6.ta:vendor/app/t6/b46325e6-5c90-8252-2eada8e32e5180d6.ta

	# kph configs
	PRODUCT_COPY_FILES += vendor/mediatek/proprietary/trustzone/trustkernel/source/bsp/platform/common/scripts/kph_cfg/cfg.ini:vendor/app/t6/cfg.ini

	VENDOR_TA :=

	# inherit project specific info
	-include vendor/mediatek/proprietary/trustzone/trustkernel/source/build/$(MTK_TARGET_PROJECT)/device_project.mk

	include vendor/mediatek/proprietary/trustzone/trustkernel/source/build/copy_vendor_ta.mk
endif
