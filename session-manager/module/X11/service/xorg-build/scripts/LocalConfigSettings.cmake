set(XOBJBASE_RELATIVE "../xorg-server/build-main")
# ubuntu <= 13.10, debian wheezy
set(ADDITIONAL_REQUIRED_LIBS "-lselinux -laudit -lgcrypt")
# ubuntu >= 13.10
#set(ADDITIONAL_REQUIRED_LIBS "-lselinux -laudit -lgcrypt -lglapi")
