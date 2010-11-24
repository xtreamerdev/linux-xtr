#ifndef __PTP_H__
#define __PTP_H__



// Operation Codes
#define PTP_OC_Undefined                0x1000
#define PTP_OC_GetDeviceInfo            0x1001
#define PTP_OC_OpenSession              0x1002
#define PTP_OC_CloseSession             0x1003
#define PTP_OC_GetStorageIDs            0x1004
#define PTP_OC_GetStorageInfo           0x1005
#define PTP_OC_GetNumObjects            0x1006
#define PTP_OC_GetObjectHandles         0x1007
#define PTP_OC_GetObjectInfo            0x1008
#define PTP_OC_GetObject                0x1009
#define PTP_OC_GetThumb                 0x100A
#define PTP_OC_DeleteObject             0x100B
#define PTP_OC_SendObjectInfo           0x100C
#define PTP_OC_SendObject               0x100D
#define PTP_OC_InitiateCapture          0x100E
#define PTP_OC_FormatStore              0x100F
#define PTP_OC_ResetDevice              0x1010
#define PTP_OC_SelfTest                 0x1011
#define PTP_OC_SetObjectProtection      0x1012
#define PTP_OC_PowerDown                0x1013
#define PTP_OC_GetDevicePropDesc        0x1014
#define PTP_OC_GetDevicePropValue       0x1015
#define PTP_OC_SetDevicePropValue       0x1016
#define PTP_OC_ResetDevicePropValue     0x1017
#define PTP_OC_TerminateOpenCapture     0x1018
#define PTP_OC_MoveObject               0x1019
#define PTP_OC_CopyObject               0x101A
#define PTP_OC_GetPartialObject         0x101B
#define PTP_OC_InitiateOpenCapture      0x101C
// Eastman Kodak extension Operation Codes
#define PTP_OC_EK_SendFileObjectInfo	0x9005
#define PTP_OC_EK_SendFileObject	0x9006



// DataType Codes 
#define PTP_DTC_UNDEF		0x0000
#define PTP_DTC_INT8		0x0001
#define PTP_DTC_UINT8		0x0002
#define PTP_DTC_INT16		0x0003
#define PTP_DTC_UINT16		0x0004
#define PTP_DTC_INT32		0x0005
#define PTP_DTC_UINT32		0x0006
#define PTP_DTC_INT64		0x0007
#define PTP_DTC_UINT64		0x0008
#define PTP_DTC_INT128		0x0009
#define PTP_DTC_UINT128		0x000A
#define PTP_DTC_AINT8		0x4001
#define PTP_DTC_AUINT8		0x4002
#define PTP_DTC_AINT16		0x4003
#define PTP_DTC_AUINT16		0x4004
#define PTP_DTC_AINT32		0x4005
#define PTP_DTC_AUINT32		0x4006
#define PTP_DTC_AINT64		0x4007
#define PTP_DTC_AUINT64		0x4008
#define PTP_DTC_AINT128		0x4009
#define PTP_DTC_AUINT128	0x400A
#define PTP_DTC_STR		0xFFFF

// max ptp string length INCLUDING terminating null character
#define PTP_MAXSTRLEN				255


// Response Codes
#define PTP_RC_Undefined                0x2000
#define PTP_RC_OK                       0x2001
#define PTP_RC_GeneralError             0x2002
#define PTP_RC_SessionNotOpen           0x2003
#define PTP_RC_InvalidTransactionID     0x2004
#define PTP_RC_OperationNotSupported    0x2005
#define PTP_RC_ParameterNotSupported    0x2006
#define PTP_RC_IncompleteTransfer       0x2007
#define PTP_RC_InvalidStorageId         0x2008
#define PTP_RC_InvalidObjectHandle      0x2009
#define PTP_RC_DevicePropNotSupported   0x200A
#define PTP_RC_InvalidObjectFormatCode  0x200B
#define PTP_RC_StoreFull                0x200C
#define PTP_RC_ObjectWriteProtected     0x200D
#define PTP_RC_StoreReadOnly            0x200E
#define PTP_RC_AccessDenied             0x200F
#define PTP_RC_NoThumbnailPresent       0x2010
#define PTP_RC_SelfTestFailed           0x2011
#define PTP_RC_PartialDeletion          0x2012
#define PTP_RC_StoreNotAvailable        0x2013
#define PTP_RC_SpecificationByFormatUnsupported         0x2014
#define PTP_RC_NoValidObjectInfo        0x2015
#define PTP_RC_InvalidCodeFormat        0x2016
#define PTP_RC_UnknownVendorCode        0x2017
#define PTP_RC_CaptureAlreadyTerminated 0x2018
#define PTP_RC_DeviceBusy               0x2019
#define PTP_RC_InvalidParentObject      0x201A
#define PTP_RC_InvalidDevicePropFormat  0x201B
#define PTP_RC_InvalidDevicePropValue   0x201C
#define PTP_RC_InvalidParameter         0x201D
#define PTP_RC_SessionAlreadyOpened     0x201E
#define PTP_RC_TransactionCanceled      0x201F
#define PTP_RC_SpecificationOfDestinationUnsupported            0x2020

#define PTP_NRC_GETOBJECT			0x20FF // not yet to get response , we read file data in readpage first 


// Eastman Kodak extension Response Codes
#define PTP_RC_EK_FilenameRequired	0xA001
#define PTP_RC_EK_FilenameConflicts	0xA002
#define PTP_RC_EK_FilenameInvalid	0xA003
// libptp2 extended ERROR codes
#define PTP_ERROR_IO			0x02FF
#define PTP_ERROR_DATA_EXPECTED		0x02FE
#define PTP_ERROR_RESP_EXPECTED		0x02FD
#define PTP_ERROR_BADPARAM		0x02FC
      
      
      
struct ptp_container
{
    __u16 code;
    __u32 sessionID;
    __u32 transactionID;
    /* params  may be of any type of size less or equal to __u32 */
    __u32 param1;
    __u32 param2;
    __u32 param3;
    /* events can only have three parameters */
    __u32 param4;
    __u32 param5;
    /* the number of meaningfull parameters */
    __u8 nparam;
};


// PTP USB Bulk-Pipe container
/* USB bulk max max packet length for high speed endpoints */
#define PTP_USB_BULK_HS_MAX_PACKET_LEN	512
#define PTP_USB_BULK_HDR_LEN		(2*sizeof(__u32)+2*sizeof(__u16))
#define PTP_USB_BULK_PAYLOAD_LEN	(PTP_USB_BULK_HS_MAX_PACKET_LEN-PTP_USB_BULK_HDR_LEN)
#define PTP_USB_BULK_REQ_LEN	        (PTP_USB_BULK_HDR_LEN+5*sizeof(__u32))

// USB container types
#define PTP_USB_CONTAINER_UNDEFINED		0x0000
#define PTP_USB_CONTAINER_COMMAND		0x0001
#define PTP_USB_CONTAINER_DATA			0x0002
#define PTP_USB_CONTAINER_RESPONSE		0x0003
#define PTP_USB_CONTAINER_EVENT			0x0004

struct ptp_usb_bulkcontainer
{
    __u32 length;
    __u16 type;
    __u16 code;
    __u32 trans_id;
    union
    {
        struct
        {
            __u32 param1;
            __u32 param2;
            __u32 param3;
            __u32 param4;
            __u32 param5;
        } params;
        unsigned char data[PTP_USB_BULK_PAYLOAD_LEN];
    } payload;
};




struct ptp_device_info
{
    __u16 standard_version;
    __u32 vendor_extensionID;
    __u16 vendor_extension_version;
    char  *vendor_extension_desc;
    __u16 functional_mode;
    __u32 operations_supported_len;
    __u16 *operations_supported;
    __u32 events_supported_len;
    __u16 *events_supported;
    __u32 device_properties_supported_len;
    __u16 *device_properties_supported;
    __u32 capture_formats_len;
    __u16 *capture_formats;
    __u32 image_formats_len;
    __u16 *image_formats;
    char  *manufacturer;
    char  *model;
    char  *device_version;
    char  *serial_number;
};




// PTP Protection Status

#define PTP_PS_NoProtection			0x0000
#define PTP_PS_ReadOnly				0x0001

// PTP Storage Types

#define PTP_ST_Undefined			0x0000
#define PTP_ST_FixedROM				0x0001
#define PTP_ST_RemovableROM			0x0002
#define PTP_ST_FixedRAM				0x0003
#define PTP_ST_RemovableRAM			0x0004

// PTP FilesystemType Values

#define PTP_FST_Undefined			0x0000
#define PTP_FST_GenericFlat			0x0001
#define PTP_FST_GenericHierarchical		0x0002
#define PTP_FST_DCF				0x0003

// PTP StorageInfo AccessCapability Values

#define PTP_AC_ReadWrite			0x0000
#define PTP_AC_ReadOnly				0x0001
#define PTP_AC_ReadOnly_with_Object_Deletion	0x0002

struct ptp_storage_ids
{
    __u32 n;
    __u32 *storage;
};

struct ptp_storage_info
{
    __u16 storage_type;
    __u16 filesystem_type;
    __u16 access_capability;
    __u64 max_capability;
    __u64 free_space_in_bytes;
    __u32 free_space_in_images;
    char  *storage_description;
    char  *volume_label;
};



// PTP Object Format Codes
// ancillary formats
#define PTP_OFC_Undefined			0x3000
#define PTP_OFC_Association			0x3001
#define PTP_OFC_Script				0x3002
#define PTP_OFC_Executable			0x3003
#define PTP_OFC_Text				0x3004
#define PTP_OFC_HTML				0x3005
#define PTP_OFC_DPOF				0x3006
#define PTP_OFC_AIFF	 			0x3007
#define PTP_OFC_WAV				0x3008
#define PTP_OFC_MP3				0x3009
#define PTP_OFC_AVI				0x300A
#define PTP_OFC_MPEG				0x300B
#define PTP_OFC_ASF				0x300C
#define PTP_OFC_QT				0x300D // guessing
// image formats
#define PTP_OFC_EXIF_JPEG			0x3801
#define PTP_OFC_TIFF_EP				0x3802
#define PTP_OFC_FlashPix			0x3803
#define PTP_OFC_BMP				0x3804
#define PTP_OFC_CIFF				0x3805
#define PTP_OFC_Undefined_0x3806		0x3806
#define PTP_OFC_GIF				0x3807
#define PTP_OFC_JFIF				0x3808
#define PTP_OFC_PCD				0x3809
#define PTP_OFC_PICT				0x380A
#define PTP_OFC_PNG				0x380B
#define PTP_OFC_Undefined_0x380C		0x380C
#define PTP_OFC_TIFF				0x380D
#define PTP_OFC_TIFF_IT				0x380E
#define PTP_OFC_JP2				0x380F
#define PTP_OFC_JPX				0x3810
// Eastman Kodak extension ancillary format
#define PTP_OFC_EK_M3U				0xb002


// Association Types
#define PTP_AT_Undefined			0x0000
#define PTP_AT_GenericFolder			0x0001
#define PTP_AT_Album				0x0002
#define PTP_AT_TimeSequence			0x0003
#define PTP_AT_HorizontalPanoramic		0x0004
#define PTP_AT_VerticalPanoramic		0x0005
#define PTP_AT_2DPanoramic			0x0006
#define PTP_AT_AncillaryData			0x0007

struct ptp_object_handles
{
    __u32 n;
    __u32 *handles;
};

struct ptp_object_info
{
    __u32 storage_id;
    __u16 object_format;
    __u16 protection_status;
    __u32 object_compressed_size;
    __u16 thumb_format;
    __u32 thumb_compressed_size;
    __u32 thumb_pix_width;
    __u32 thumb_pix_height;
    __u32 image_pix_width;
    __u32 image_pix_height;
    __u32 image_bit_depth;
    __u32 parent_object;
    __u16 association_type;
    __u32 association_desc;
    __u32 sequence_number;
    char    *filename;
    time_t  capture_date;
    time_t  modification_date;
    char    *keywords;
};



// Device Property Form Flag
#define PTP_DPFF_None			0x00
#define PTP_DPFF_Range			0x01
#define PTP_DPFF_Enumeration		0x02
// Device Property GetSet type
#define PTP_DPGS_Get			0x00
#define PTP_DPGS_GetSet			0x01

struct ptp_prop_desc_range_form
{
    void * minimum_value;
    void * maximum_value;
    void * step_size;
};
// _property _describing _dataset, _enum _form
struct ptp_prop_desc_enum_form
{
    __u16   number_of_values;
    void ** supported_value;
};
// _device _property _describing _dataset (_device_prop_desc)
struct ptp_device_prop_desc
{
    __u16 device_property_code;
    __u16 data_type;
    __u8  get_set;
    void * factory_default_value;
    void * current_value;
    __u8 form_flag;
    union
    {
        struct ptp_prop_desc_enum_form  menum;
        struct ptp_prop_desc_range_form range;
    } form;
};


// for transfering data in and out.  Maximum kmalloc size is around 128K
// so we need to break things into smaller chunks
struct ptp_data_buffer {
    struct ptp_block {
        int block_size;
        unsigned char *block;
    } *blocks;
	int num_seg; 				//	total blocks that should be read from DSC
	/*
	**	if num_seg = 4, num_blocks must be 1, 2, 3, or 4.
	*/
	int num_blocks;  			//	already read blocks from DSC
	/*
	**	record which block be read now, if MAX_SEG_NUM = 6 ,record_blocks must be 0~5
	**	ex: buf2->blocks[buf2->record_blocks].block
	*/
	int record_blocks;		
		
	int offset;		//	record the last readpage offset
	int count;			//	record how much block(bytes) already be read to pages, if offset is in 3rd_block,count = 1st_block_size + 2nd_block_size

    //for get DATA,get_response will be executed in readpage
    struct ptpfs_sb_info *sb_temp;
    struct ptp_container *ptp_temp;
};

#endif
