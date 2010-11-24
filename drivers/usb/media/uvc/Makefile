
obj-$(CONFIG_USB_UVC)		+= uvcvideo.o
uvcvideo-y	:= uvc_driver.o uvc_queue.o uvc_v4l2.o uvc_video.o uvc_ctrl.o uvc_status.o uvc_isight.o


clean:
	-rm -f *.o *.ko .*.cmd .*.flags *.mod.c Module.symvers 
	-rm -rf .tmp_versions

