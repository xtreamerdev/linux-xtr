#define _XP_OS_INTFS_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>



void xp_stop_drv_threads (_adapter *padapter){


	padapter->bDriverStopped = _TRUE;
	padapter->bSurpriseRemoved = _TRUE;
_func_enter_;
		
	//4 cmd_thread
	if (padapter->cmdThread != NULL)
	{
		DEBUG_INFO(("stop_drv_threads() : wait for cmdThread terminating.\n"));

		KeWaitForSingleObject(padapter->cmdThread, Executive, KernelMode, _FALSE, NULL);
		ObDereferenceObject(padapter->cmdThread);

		DEBUG_INFO(("stop_drv_threads() : cmdThread terminated.\n"));
	}	



	//4 event_thread
	if (padapter->evtThread != NULL)
	{
		DEBUG_INFO(("stop_drv_threads() : wait for evtThread terminating.\n"));

		KeWaitForSingleObject(padapter->evtThread, Executive, KernelMode, _FALSE, NULL);
		ObDereferenceObject(padapter->evtThread);

		DEBUG_INFO(("stop_drv_threads() : evtThread terminated.\n"));
	}	

#ifndef  use_xmit_func
	//4 xmit_thread
	if (padapter->xmitThread != NULL)
	{
		DEBUG_INFO(("stop_drv_threads() : wait for xmitThread terminating.\n"));

		KeWaitForSingleObject(padapter->xmitThread, Executive, KernelMode, _FALSE, NULL);
		ObDereferenceObject(padapter->xmitThread);

		DEBUG_INFO(("stop_drv_threads() : xmitThread terminated.\n"));
	}	
#endif
#ifndef  use_recv_func
	//4 recv_thread
	if (padapter->recvThread != NULL)
	{
		DEBUG_INFO(("stop_drv_threads() : wait for recvThread terminating.\n"));

		KeWaitForSingleObject(padapter->recvThread, Executive, KernelMode, _FALSE, NULL);
		ObDereferenceObject(padapter->recvThread);

		DEBUG_INFO(("stop_drv_threads() : recvThread terminated.\n"));
	}	
#endif
_func_exit_;	
}


u32 xp_start_drv_threads(_adapter *padapter){

	NTSTATUS status;
	HANDLE hThread;
	OBJECT_ATTRIBUTES oa;
	 u32	ret=_SUCCESS;

_func_enter_;
	//4 cmd_thread
	InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
	status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, &oa, NULL, NULL, (PKSTART_ROUTINE)cmd_thread,padapter);
	
	if (!NT_SUCCESS(status))
	{
		DEBUG_ERR(("Create thread fail: cmd_thread .\n"));
		ret=_FAIL;
		goto exit;
	}

	ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID *)&padapter->cmdThread, NULL);
	ZwClose(hThread);

	//4 event_thread
	InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
	status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, &oa, NULL, NULL, (PKSTART_ROUTINE)event_thread, padapter);
	if (!NT_SUCCESS(status))
	{
		DEBUG_ERR(("Create thread fail: event_thread .\n"));
		ret=_FAIL;
		goto exit;
	}

	ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID *)&padapter->evtThread, NULL);
	ZwClose(hThread);

	//4 xmit_thread
	InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
	status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, &oa, NULL, NULL, (PKSTART_ROUTINE)xmit_thread, padapter);
	if (!NT_SUCCESS(status))
	{
		DEBUG_ERR(("Create thread fail: xmit_thread .\n"));
		ret=_FAIL;
		goto exit;
	}

	ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID *)&padapter->xmitThread, NULL);
	ZwClose(hThread);

	//4 recv_thread
	InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);	
	status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, &oa, NULL, NULL, (PKSTART_ROUTINE)recv_thread, padapter);
	if (!NT_SUCCESS(status))
	{
		DEBUG_ERR((" Create thread fail: recv_thread .\n"));
		ret=_FAIL;
		goto exit;
	}

	ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID *)&padapter->recvThread, NULL);
	ZwClose(hThread);
exit:	
	if (ret == _FAIL) 
	{
		padapter->bDriverStopped = _TRUE;
		padapter->bSurpriseRemoved = _TRUE;		
		xp_stop_drv_threads(padapter);

	}
_func_exit_;	
	return ret;
}

