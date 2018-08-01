BFN_SAI = bfnsdk_1.0.0_amd64.deb
$(BFN_SAI)_URL = "http://192.168.204.19/bfnsdk_1.0.0_amd64.deb"
#$(BFN_SAI)_URL = "http://10.12.21.133/sagar/bfnsdk_1.0.0_amd64.deb"


SONIC_ONLINE_DEBS += $(BFN_SAI) # $(BFN_SAI_DEV)
$(BFN_SAI_DEV)_DEPENDS += $(BFN_SAI)
