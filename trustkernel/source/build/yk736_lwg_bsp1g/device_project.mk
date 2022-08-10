# !!! DO NOT use PRODUCT_COPY_FILES to copy vendor TA!!!

ifeq ($(strip $(TRUSTKERNEL_TEE_SUPPORT)), yes)
    # project specific trusted applications
    VENDOR_TA += vendor/mediatek/proprietary/trustzone/trustkernel/source/ta/vendor/$(MTK_TARGET_PROJECT)/5b9e0e41-2636-11e1-ad9e0002a5d5c51b.ta

    # project specific configurations
endif
