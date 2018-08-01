include $(PLATFORM_PATH)/platform-modules-bfn.mk
include $(PLATFORM_PATH)/platform-modules-bfn-montara.mk
include $(PLATFORM_PATH)/platform-modules-rj-s6500.mk
include $(PLATFORM_PATH)/platform-modules-rj-f9500.mk
include $(PLATFORM_PATH)/bfn-sai.mk
include $(PLATFORM_PATH)/docker-syncd-bfn.mk
include $(PLATFORM_PATH)/docker-syncd-bfn-rpc.mk
include $(PLATFORM_PATH)/docker-orchagent-bfn.mk
include $(PLATFORM_PATH)/one-image.mk
include $(PLATFORM_PATH)/libsaithrift-dev.mk
include $(PLATFORM_PATH)/python-saithrift.mk
include $(PLATFORM_PATH)/docker-ptf-bfn.mk
include $(PLATFORM_PATH)/bfn-platform.mk
SONIC_ALL += $(SONIC_ONE_IMAGE) \
             $(DOCKER_FPM)

# Inject sai into sairedis
$(LIBSAIREDIS)_DEPENDS += $(BFN_SAI) $(BFN_PLATFORM)  #$(LIBSAITHRIFT_DEV_BFN)

# Runtime dependency on sai is set only for syncd
$(SYNCD)_RDEPENDS += $(BFN_SAI) $(BFN_PLATFORM)
