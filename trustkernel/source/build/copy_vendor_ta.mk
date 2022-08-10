ifeq ($(strip $(TRUSTKERNEL_TEE_SUPPORT)), yes)

    # verify certificate
    #$(if $(filter VALIDCERT,$(shell vendor/mediatek/proprietary/trustzone/trustkernel/source/tools/dump_project_cert.py --silent \
            --type evaluate,limited,builtin,volume --cert vendor/mediatek/proprietary/trustzone/trustkernel/source/build/$(MTK_TARGET_PROJECT)/cert.dat)), \
            ,$(error vendor/mediatek/proprietary/trustzone/trustkernel/source/build/$(MTK_TARGET_PROJECT)/cert.dat CANNOT be validated!!!))
    
    # verify vendor TA
    #$(foreach ta, $(VENDOR_TA), \
        $(if $(filter VALIDTA,$(shell vendor/mediatek/proprietary/trustzone/trustkernel/source/tools/verify.py --silent \
            --project_cert vendor/mediatek/proprietary/trustzone/trustkernel/source/build/$(MTK_TARGET_PROJECT)/cert.dat --in $(ta))), \
            $(eval PRODUCT_COPY_FILES += $(ta):system/app/t6/$(notdir $(ta))), $(error $(ta) CANNOT be verified!!!)))
endif
