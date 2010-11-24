// curently this file is included into ptp.c

//detect if data is split accross blocks and copy to buf is need be
//assumes data is relatively small and will only split one block

static unsigned char *get_charptr(struct ptp_data_buffer *data, int pos, int len, unsigned char *buf) 
{
    int block = 0;

    while (pos >= data->blocks[block].block_size)
    {
        pos -= data->blocks[block].block_size;
        block++;
    }
    if ((pos+len) < data->blocks[block].block_size)
    {
        return &data->blocks[block].block[pos];
    }

    // data splits across two blocks, need to copy to the tmp buffer
    memcpy(buf,&data->blocks[block].block[pos],data->blocks[block].block_size-pos);

	if(data->num_blocks > 1)
	{
	    memcpy(&buf[data->blocks[block].block_size-pos],data->blocks[block+1].block,len-(data->blocks[block].block_size-pos));
	}

	return buf;
}


static void write_data(struct ptp_data_buffer *data, int pos, int len, unsigned char *buf) {
    int block = 0;
    while (pos >= data->blocks[block].block_size)
    {
        pos -= data->blocks[block].block_size;
        block++;
    }
    if ((pos+len) < data->blocks[block].block_size)
    {
        memcpy(&data->blocks[block].block[pos],buf,len);
        return;
    }
    memcpy(&data->blocks[block].block[pos],buf,data->blocks[block].block_size-pos);
    pos += data->blocks[block].block_size-pos;
    block++;

    while ((pos+len) > data->blocks[block].block_size)
    {
        memcpy(data->blocks[block].block,&buf[pos],data->blocks[block].block_size);
        pos += data->blocks[block].block_size;
        len -= data->blocks[block].block_size;
        block++;
    }
    memcpy(data->blocks[block].block,&buf[data->blocks[block-1].block_size-pos],len-(data->blocks[block-1].block_size-pos));
}
            

static inline __u16 htod16p (struct ptpfs_sb_info *sb, __u16 var)
{
    return((sb->byteorder==PTP_DL_LE)?cpu_to_le16(var):cpu_to_be16(var));
}

static inline __u32 htod32p (struct ptpfs_sb_info *sb, __u32 var)
{
    return((sb->byteorder==PTP_DL_LE)?cpu_to_le32(var):cpu_to_be32(var));
}

static inline void htod8ap (struct ptpfs_sb_info *sb, struct ptp_data_buffer *data, int offset, __u8 val)
{
    write_data(data,offset,sizeof(val),(unsigned char *)&val);
}
static inline void htod16ap (struct ptpfs_sb_info *sb, struct ptp_data_buffer *data, int offset, __u16 val)
{
    __u16 b;
    if (sb->byteorder==PTP_DL_LE)
        b = cpu_to_le16(val);
    else
        b = cpu_to_be16(val);
    write_data(data,offset,sizeof(b),(unsigned char *)&b);
}

static inline void htod32ap (struct ptpfs_sb_info *sb, struct ptp_data_buffer *data, int offset, __u32 val)
{
    __u32 b;
    if (sb->byteorder==PTP_DL_LE)
        b = cpu_to_le32(val);
    else
        b = cpu_to_be32(val);
    write_data(data,offset,sizeof(b),(unsigned char *)&b);
}



#define dtoh8a(x)	(*(__u8*)(x))

static inline __u16 dtoh16p (struct ptpfs_sb_info *sb, __u16 var)
{
    return((sb->byteorder==PTP_DL_LE)?le16_to_cpu(var):be16_to_cpu(var));
}
static inline __u32 dtoh32p (struct ptpfs_sb_info *sb, __u32 var)
{
    return((sb->byteorder==PTP_DL_LE)?le32_to_cpu(var):be32_to_cpu(var));
}
static inline __u64 dtoh64p (struct ptpfs_sb_info *sb, __u64 var)
{
    return((sb->byteorder==PTP_DL_LE)?le64_to_cpu(var):be64_to_cpu(var));
}


#define dtoh8ap(a,x)	(*(__u8*)(x))
static inline __u16 dtoh16ap (struct ptpfs_sb_info *sb, unsigned char *a)
{
    __u16 b;
    __u16 *p = (__u16 *)a;
    b = get_unaligned(p);
    return((sb->byteorder==PTP_DL_LE)?le16_to_cpu(b):be16_to_cpu(b));
}
static inline __u32 dtoh32ap (struct ptpfs_sb_info *sb, unsigned char *a)
{
    __u32 b;
    __u32 *p = (__u32 *)a;
    b = get_unaligned(p);
    return((sb->byteorder==PTP_DL_LE)?le32_to_cpu(b):be32_to_cpu(b));
}
static inline __u64 dtoh64ap (struct ptpfs_sb_info *sb, unsigned char *a)
{
    __u64 b;
    __u64 *p = (__u64 *)a;
    b = get_unaligned(p);
    return((sb->byteorder==PTP_DL_LE)?le64_to_cpu(b):be64_to_cpu(b));
}


static inline __u8 dtoh8apd (struct ptpfs_sb_info *sb, struct ptp_data_buffer *data, int offset)
{
    char buf[1];
    return dtoh8ap(sb,get_charptr(data,offset,1,buf));
}
static inline __u16 dtoh16apd (struct ptpfs_sb_info *sb, struct ptp_data_buffer *data, int offset)
{
    char buf[2];
    return dtoh16ap(sb,get_charptr(data,offset,2,buf));
}

static inline __u32 dtoh32apd (struct ptpfs_sb_info *sb, struct ptp_data_buffer *data, int offset)
{
    char buf[4];
    return dtoh32ap(sb,get_charptr(data,offset,4,buf));
}
static inline __u64 dtoh64apd (struct ptpfs_sb_info *sb, struct ptp_data_buffer *data, int offset)
{
    char buf[8];
    return dtoh64ap(sb,get_charptr(data,offset,8,buf));
}




static inline char*
ptp_unpack_string(struct ptpfs_sb_info *sb, struct ptp_data_buffer *data, __u16 offset, __u8 *len)
{
    int i;
    char *string=NULL;
    unsigned char buf[2];

    *len=dtoh8a(get_charptr(data, offset, 1, buf));
    if (*len)
    {
        string=kmalloc(*len, GFP_KERNEL);
        memset(string, 0, *len);
        for (i=0;i<*len && i< PTP_MAXSTRLEN; i++)
        {
            string[i]=(char)dtoh16ap(sb,get_charptr(data, offset+i*2+1, 2, buf));
        }
        // be paranoid! :(
        string[*len-1]=0;
    }
    return(string);
}

static inline void
ptp_pack_string(struct ptpfs_sb_info *sb, char *string, struct ptp_data_buffer *data, __u16 offset, __u8 *len)
{
    int i;
    *len = (__u8)strlen(string);
    i=*len+1;
    // XXX: check strlen!
    htod8ap(sb,data,offset,(__u8)i);
    for (i=0;i<*len && i< PTP_MAXSTRLEN; i++)
    {
        htod16ap(sb,data,offset+i*2+1,(__u16)string[i]);
    }
}

static inline __u32
ptp_unpack___u32_array(struct ptpfs_sb_info *sb, struct ptp_data_buffer *data, __u16 offset, __u32 **array)
{
    unsigned char buf[4];
    __u32 n, i=0;

    n=dtoh32ap(sb,get_charptr(data, offset, sizeof(__u32), buf));
    *array = kmalloc (n*sizeof(__u32), GFP_KERNEL);
    while (n>i)
    {
        (*array)[i]=dtoh32ap(sb,get_charptr(data, offset+(sizeof(__u32)*(i+1)), sizeof(__u32), buf));
        i++;
    }
    return n;
}

static inline __u32
ptp_unpack___u16_array(struct ptpfs_sb_info *sb, struct ptp_data_buffer *data, __u16 offset, __u16 **array)
{
    __u32 n, i=0;
    unsigned char buf[4];

    n=dtoh32ap(sb,get_charptr(data, offset, sizeof(__u32), buf));
    *array = kmalloc(n*sizeof(__u16), GFP_KERNEL);
    while (n>i)
    {
        (*array)[i]=dtoh16ap(sb,get_charptr(data, offset+(sizeof(__u16)*(i+2)), sizeof(__u32), buf));
        i++;
    }
    return n;
}

// DeviceInfo pack/unpack

#define PTP_di_StandardVersion		 0
#define PTP_di_VendorExtensionID	 2
#define PTP_di_VendorExtensionVersion	 6
#define PTP_di_VendorExtensionDesc	 8
#define PTP_di_FunctionalMode		 8
#define PTP_di_OperationsSupported	10

static inline void
ptp_unpack_DI (struct ptpfs_sb_info *sb, struct ptp_data_buffer *data, struct ptp_device_info *di)
{
    __u8 len;
    unsigned int totallen;

    di->standard_version = dtoh16apd(sb,data,PTP_di_StandardVersion);
    di->vendor_extensionID = dtoh32apd(sb,data,PTP_di_VendorExtensionID);
    di->vendor_extension_version = dtoh16apd(sb,data,PTP_di_VendorExtensionVersion);
    di->vendor_extension_desc = ptp_unpack_string(sb, data,PTP_di_VendorExtensionDesc, &len); 
    totallen=len*2+1;
    di->functional_mode = dtoh16apd(sb,data,PTP_di_FunctionalMode+totallen);
    di->operations_supported_len = ptp_unpack___u16_array(sb, data, PTP_di_OperationsSupported+totallen,&di->operations_supported);
    totallen=totallen+di->operations_supported_len*sizeof(__u16)+sizeof(__u32);
    di->events_supported_len = ptp_unpack___u16_array(sb, data, PTP_di_OperationsSupported+totallen,&di->events_supported);
    totallen=totallen+di->events_supported_len*sizeof(__u16)+sizeof(__u32);
    di->device_properties_supported_len = ptp_unpack___u16_array(sb, data, PTP_di_OperationsSupported+totallen,&di->device_properties_supported);
    totallen=totallen+di->device_properties_supported_len*sizeof(__u16)+sizeof(__u32);
    di->capture_formats_len = ptp_unpack___u16_array(sb, data,PTP_di_OperationsSupported+totallen,&di->capture_formats);
    totallen=totallen+di->capture_formats_len*sizeof(__u16)+sizeof(__u32);
    di->image_formats_len = ptp_unpack___u16_array(sb, data,PTP_di_OperationsSupported+totallen,&di->image_formats);
    totallen=totallen+di->image_formats_len*sizeof(__u16)+sizeof(__u32);
    di->manufacturer = ptp_unpack_string(sb, data, PTP_di_OperationsSupported+totallen, &len);
    totallen+=len*2+1;
    di->model = ptp_unpack_string(sb, data,PTP_di_OperationsSupported+totallen,&len);
    totallen+=len*2+1;
    di->device_version = ptp_unpack_string(sb, data,PTP_di_OperationsSupported+totallen,&len);
    totallen+=len*2+1;
    di->serial_number = ptp_unpack_string(sb, data,PTP_di_OperationsSupported+totallen,&len);
}

// ObjectHandles array pack/unpack
static inline void ptp_unpack_OH (struct ptpfs_sb_info *sb, struct ptp_data_buffer *data, struct ptp_object_handles *oh)
{
    oh->n = ptp_unpack___u32_array(sb, data, 0, &oh->handles);
}

// StoreIDs array pack/unpack

static inline void ptp_unpack_SIDs (struct ptpfs_sb_info *sb, struct ptp_data_buffer *data, struct ptp_storage_ids *sids)
{
    sids->n = ptp_unpack___u32_array(sb, data, 0,&sids->storage);
}

// StorageInfo pack/unpack

#define PTP_si_StorageType		 0
#define PTP_si_FilesystemType		 2
#define PTP_si_AccessCapability		 4
#define PTP_si_MaxCapability		 6
#define PTP_si_FreeSpaceInBytes		14
#define PTP_si_FreeSpaceInImages	22
#define PTP_si_StorageDescription	26

static inline void  ptp_unpack_SI (struct ptpfs_sb_info *sb, struct ptp_data_buffer *data, struct ptp_storage_info *si)
{
    __u8 storagedescriptionlen;

    si->storage_type=dtoh16apd(sb,data,PTP_si_StorageType);
    si->filesystem_type=dtoh16apd(sb,data,PTP_si_FilesystemType);
    si->access_capability=dtoh16apd(sb,data,PTP_si_AccessCapability);
    si->max_capability=dtoh64apd(sb,data,PTP_si_MaxCapability);
    si->free_space_in_bytes=dtoh64apd(sb,data,PTP_si_FreeSpaceInBytes);
    si->free_space_in_images=dtoh32apd(sb,data,PTP_si_FreeSpaceInImages);
    si->storage_description=ptp_unpack_string(sb, data,PTP_si_StorageDescription, &storagedescriptionlen);
    si->volume_label=ptp_unpack_string(sb, data,PTP_si_StorageDescription+storagedescriptionlen*2+1,&storagedescriptionlen);
}

// ObjectInfo pack/unpack
#define PTP_oi_StorageID		 0
#define PTP_oi_ObjectFormat		 4
#define PTP_oi_ProtectionStatus		 6
#define PTP_oi_ObjectCompressedSize	 8
#define PTP_oi_ThumbFormat		12
#define PTP_oi_ThumbCompressedSize	14
#define PTP_oi_ThumbPixWidth		18
#define PTP_oi_ThumbPixHeight		22
#define PTP_oi_ImagePixWidth		26
#define PTP_oi_ImagePixHeight		30
#define PTP_oi_ImageBitDepth		34
#define PTP_oi_ParentObject		38
#define PTP_oi_AssociationType		42
#define PTP_oi_AssociationDesc		44
#define PTP_oi_SequenceNumber		48
#define PTP_oi_filenamelen		52
#define PTP_oi_Filename			53

static inline __u32 ptp_pack_OI (struct ptpfs_sb_info *sb, struct ptp_object_info *oi, unsigned char** oidataptr)
{
    struct ptp_data_buffer data;
    struct ptp_block block;

    unsigned char* oidata;
    __u8 filenamelen;
    __u8 capturedatelen=0;

    data.num_blocks = 1;
    block.block_size = PTP_oi_Filename+(strlen(oi->filename)+1)*2+4;
    /* let's allocate some memory first; XXX i'm sure it's wrong */
    oidata=kmalloc(block.block_size, GFP_KERNEL);
    // the caller should kfree it after use!
    memset(oidata, 0, block.block_size);
    block.block = oidata;
    data.blocks=&block;


    htod32ap(sb,&data,PTP_oi_StorageID,oi->storage_id);
    htod16ap(sb,&data,PTP_oi_ObjectFormat,oi->object_format);
    htod16ap(sb,&data,PTP_oi_ProtectionStatus,oi->protection_status);
    htod32ap(sb,&data,PTP_oi_ObjectCompressedSize,oi->object_compressed_size);
    htod16ap(sb,&data,PTP_oi_ThumbFormat,oi->thumb_format);
    htod32ap(sb,&data,PTP_oi_ThumbCompressedSize,oi->thumb_compressed_size);
    htod32ap(sb,&data,PTP_oi_ThumbPixWidth,oi->thumb_pix_width);
    htod32ap(sb,&data,PTP_oi_ThumbPixHeight,oi->thumb_pix_height);
    htod32ap(sb,&data,PTP_oi_ImagePixWidth,oi->image_pix_width);
    htod32ap(sb,&data,PTP_oi_ImagePixHeight,oi->image_pix_height);
    htod32ap(sb,&data,PTP_oi_ImageBitDepth,oi->image_bit_depth);
    htod32ap(sb,&data,PTP_oi_ParentObject,oi->parent_object);
    htod16ap(sb,&data,PTP_oi_AssociationType,oi->association_type);
    htod32ap(sb,&data,PTP_oi_AssociationDesc,oi->association_desc);
    htod32ap(sb,&data,PTP_oi_SequenceNumber,oi->sequence_number);

    ptp_pack_string(sb, oi->filename, &data, PTP_oi_filenamelen, &filenamelen);
    /*
        filenamelen=(__u8)strlen(oi->Filename);
        htod8a(&req->data[PTP_oi_filenamelen],filenamelen+1);
        for (i=0;i<filenamelen && i< PTP_MAXSTRLEN; i++) {
            req->data[PTP_oi_Filename+i*2]=oi->Filename[i];
        }
    */
    /*
     *XXX Fake date.
     * for example Kodak sets Capture date on the basis of EXIF data.
     * Spec says that this field is from perspective of Initiator.
     */
#if 0	// seems now we don't need any data packed in OI dataset... for now ;)
    capturedatelen=strlen(capture_date);
    htod8a(&data[PTP_oi_Filename+(filenamelen+1)*2],
           capturedatelen+1);
    for (i=0;i<capturedatelen && i< PTP_MAXSTRLEN; i++)
    {
        data[PTP_oi_Filename+(i+filenamelen+1)*2+1]=capture_date[i];
    }
    htod8a(&data[PTP_oi_Filename+(filenamelen+capturedatelen+2)*2+1],
           capturedatelen+1);
    for (i=0;i<capturedatelen && i< PTP_MAXSTRLEN; i++)
    {
        data[PTP_oi_Filename+(i+filenamelen+capturedatelen+2)*2+2]=
        capture_date[i];
    }
#endif
    // XXX this function should return dataset length

    *oidataptr=oidata;
    return(PTP_oi_Filename+(filenamelen+1)*2+(capturedatelen+1)*4);
}



static int ptp_atoi(const char *s)
{
    int k = 0;

    k = 0;
    while (*s != '\0' && *s >= '0' && *s <= '9')
    {
        k = 10 * k + (*s - '0');
        s++;
    }
    return k;
}

static inline void  ptp_unpack_OI (struct ptpfs_sb_info *sb, struct ptp_data_buffer *data, struct ptp_object_info *oi)
{
    __u8 filenamelen;
    __u8 capturedatelen;
    char *capture_date;
    char tmp[16];

    unsigned int year = 0; 
    unsigned int mon = 0;
    unsigned int day = 0; 
    unsigned int hour = 0;
    unsigned int min = 0; 
    unsigned int sec = 0;

    oi->storage_id=dtoh32apd(sb,data,PTP_oi_StorageID);
    oi->object_format=dtoh16apd(sb,data,PTP_oi_ObjectFormat);
    oi->protection_status=dtoh16apd(sb,data,PTP_oi_ProtectionStatus);
    oi->object_compressed_size=dtoh32apd(sb,data,PTP_oi_ObjectCompressedSize);
    oi->thumb_format=dtoh16apd(sb,data,PTP_oi_ThumbFormat);
    oi->thumb_compressed_size=dtoh32apd(sb,data,PTP_oi_ThumbCompressedSize);
    oi->thumb_pix_width=dtoh32apd(sb,data,PTP_oi_ThumbPixWidth);
    oi->thumb_pix_height=dtoh32apd(sb,data,PTP_oi_ThumbPixHeight);
    oi->image_pix_width=dtoh32apd(sb,data,PTP_oi_ImagePixWidth);
    oi->image_pix_height=dtoh32apd(sb,data,PTP_oi_ImagePixHeight);
    oi->image_bit_depth=dtoh32apd(sb,data,PTP_oi_ImageBitDepth);
    oi->parent_object=dtoh32apd(sb,data,PTP_oi_ParentObject);
    oi->association_type=dtoh16apd(sb,data,PTP_oi_AssociationType);
    oi->association_desc=dtoh32apd(sb,data,PTP_oi_AssociationDesc);
    oi->sequence_number=dtoh32apd(sb,data,PTP_oi_SequenceNumber);
    oi->filename= ptp_unpack_string(sb, data, PTP_oi_filenamelen, &filenamelen);
    capture_date = ptp_unpack_string(sb, data, PTP_oi_filenamelen+filenamelen*2+1, &capturedatelen);

    // subset of ISO 8601, without '.s' tenths of second and
    // time zone
    if (capturedatelen>15)
    {
        strncpy (tmp, capture_date, 4);
        tmp[4] = 0;
        year=ptp_atoi (tmp);
        strncpy (tmp, capture_date + 4, 2);
        tmp[2] = 0;
        mon = ptp_atoi (tmp);
        strncpy (tmp, capture_date + 6, 2);
        tmp[2] = 0;
        day = ptp_atoi (tmp);
        strncpy (tmp, capture_date + 9, 2);
        tmp[2] = 0;
        hour = ptp_atoi (tmp);
        strncpy (tmp, capture_date + 11, 2);
        tmp[2] = 0;
        min = ptp_atoi (tmp);
        strncpy (tmp, capture_date + 13, 2);
        tmp[2] = 0;
        sec = ptp_atoi (tmp);
        oi->capture_date=mktime(year, mon, day, hour, min, sec);
    } else {
        oi->modification_date=0;
    }
    kfree(capture_date);

    // now it's modification date ;)
    capture_date = ptp_unpack_string(sb, data,
                                     PTP_oi_filenamelen+filenamelen*2
                                     +capturedatelen*2+2,&capturedatelen);
    if (capturedatelen>15)
    {
        strncpy (tmp, capture_date, 4);
        tmp[4] = 0;
        year=ptp_atoi (tmp);
        strncpy (tmp, capture_date + 4, 2);
        tmp[2] = 0;
        mon = ptp_atoi (tmp);
        strncpy (tmp, capture_date + 6, 2);
        tmp[2] = 0;
        day = ptp_atoi (tmp);
        strncpy (tmp, capture_date + 9, 2);
        tmp[2] = 0;
        hour = ptp_atoi (tmp);
        strncpy (tmp, capture_date + 11, 2);
        tmp[2] = 0;
        min = ptp_atoi (tmp);
        strncpy (tmp, capture_date + 13, 2);
        tmp[2] = 0;
        sec = ptp_atoi (tmp);
        oi->modification_date=mktime(year, mon, day, hour, min, sec);
    } else {
        oi->modification_date=0;
    }
    kfree(capture_date);
}

// Custom Type Value Assignement (without Length) macro frequently used below
#define CTVAL(type,func,target)  {					\
		target = kmalloc(sizeof(type), GFP_KERNEL);				\
		*(type *)target =					\
			func(data);\
}

/*
static inline void
ptp_unpack_DPV (struct ptpfs_sb_info *sb, struct ptp_data_buffer *data, void* value, __u16 datatype)
{

    switch (datatype)
    {
    case PTP_DTC_INT8:
        CTVAL(int8_t,dtoh8a,value);
        break;
    case PTP_DTC_UINT8:
        CTVAL(__u8,dtoh8a,value);
        break;
    case PTP_DTC_INT16:
        CTVAL(int16_t,dtoh16a,value);
        break;
    case PTP_DTC_UINT16:
        CTVAL(__u16,dtoh16a,value);
        break;
    case PTP_DTC_INT32:
        CTVAL(int32_t,dtoh32a,value);
        break;
    case PTP_DTC_UINT32:
        CTVAL(__u32,dtoh32a,value);
        break;
        // XXX: other int types are unimplemented 
        // XXX: int arrays are unimplemented also 
    case PTP_DTC_STR:
        {
            __u8 len;
            (char *)value = ptp_unpack_string (sb,data,0,&len);
            break;
        }
    }
}
static inline __u32
ptp_pack_DPV (struct ptpfs_sb_info *sb, void* value, char** dpvptr, __u16 datatype)
{
    char* dpv=NULL;
    __u32 size=0;

    switch (datatype)
    {
    case PTP_DTC_INT8:
        size=sizeof(int8_t);
        dpv=kmalloc(size, GFP_KERNEL);
        htod8ap(sb,dpv,*(int8_t*)value);
        break;
    case PTP_DTC_UINT8:
        size=sizeof(__u8);
        dpv=kmalloc(size, GFP_KERNEL);
        htod8ap(sb,dpv,*(__u8*)value);
        break;
    case PTP_DTC_INT16:
        size=sizeof(int16_t);
        dpv=kmalloc(size, GFP_KERNEL);
        htod16ap(sb,dpv,*(int16_t*)value);
        break;
    case PTP_DTC_UINT16:
        size=sizeof(__u16);
        dpv=kmalloc(size, GFP_KERNEL);
        htod16ap(sb,dpv,*(__u16*)value);
        break;
    case PTP_DTC_INT32:
        size=sizeof(int32_t);
        dpv=kmalloc(size, GFP_KERNEL);
        htod32ap(sb,dpv,*(int32_t*)value);
        break;
    case PTP_DTC_UINT32:
        size=sizeof(__u32);
        dpv=kmalloc(size, GFP_KERNEL);
        htod32ap(sb,dpv,*(__u32*)value);
        break;
        //XXX: other int types are unimplemented
        //XXX: int arrays are unimplemented also
    case PTP_DTC_STR:
        {
            __u8 len;
            size=strlen((char*)value)*2+3;
            dpv=kmalloc(size, GFP_KERNEL);
            memset(dpv,0,size);
            ptp_pack_string(sb, (char *)value, dpv, 0, &len);
        }
        break;
    }
    *dpvptr=dpv;
    return size;
}
*/


// Device Property pack/unpack
#define PTP_dpd_DevicePropertyCode	0
#define PTP_dpd_DataType		2
#define PTP_dpd_GetSet			4
#define PTP_dpd_FactoryDefaultValue	5

// Custom Type Value Assignement macro frequently used below
#define CTVA(type,func,target)  {					\
		target = kmalloc(sizeof(type), GFP_KERNEL);				\
		*(type *)target =					\
			func(sb,data,PTP_dpd_FactoryDefaultValue+totallen);\
			totallen+=sizeof(type);				\
}

// Many Custom Types Vale Assignement macro frequently used below
#define MCTVA(type,func,target,n) {					\
		__u16 i;						\
		for (i=0;i<n;i++) {					\
			target[i] = kmalloc(sizeof(type), GFP_KERNEL);		\
			*(type *)target[i] =				\
			func(sb,data,PTP_dpd_FactoryDefaultValue+totallen);\
			totallen+=sizeof(type);				\
		}							\
}

static inline void
ptp_unpack_DPD (struct ptpfs_sb_info *sb, struct ptp_data_buffer *data, struct ptp_device_prop_desc *dpd)
{
    __u8 len;
    int totallen=0;

    dpd->device_property_code=dtoh16apd(sb,data,PTP_dpd_DevicePropertyCode);
    dpd->data_type=dtoh16apd(sb,data,PTP_dpd_DataType);
    dpd->get_set=dtoh8apd(sb,data,PTP_dpd_GetSet);
    dpd->factory_default_value = NULL;
    dpd->current_value = NULL;
    switch (dpd->data_type)
    {
    case PTP_DTC_INT8:
        CTVA(int8_t,dtoh8apd,dpd->factory_default_value);
        CTVA(int8_t,dtoh8apd,dpd->current_value);
        break;
    case PTP_DTC_UINT8:
        CTVA(__u8,dtoh8apd,dpd->factory_default_value);
        CTVA(__u8,dtoh8apd,dpd->current_value);
        break;
    case PTP_DTC_INT16:
        CTVA(int16_t,dtoh16apd,dpd->factory_default_value);
        CTVA(int16_t,dtoh16apd,dpd->current_value);
        break;
    case PTP_DTC_UINT16:
        CTVA(__u16,dtoh16apd,dpd->factory_default_value);
        CTVA(__u16,dtoh16apd,dpd->current_value);
        break;
    case PTP_DTC_INT32:
        CTVA(int32_t,dtoh32apd,dpd->factory_default_value);
        CTVA(int32_t,dtoh32apd,dpd->current_value);
        break;
    case PTP_DTC_UINT32:
        CTVA(__u32,dtoh32apd,dpd->factory_default_value);
        CTVA(__u32,dtoh32apd,dpd->current_value);
        break;
        // XXX: other int types are unimplemented 
        // XXX: int arrays are unimplemented also 
    case PTP_DTC_STR:
        (char *)dpd->factory_default_value = ptp_unpack_string(sb,data,PTP_dpd_FactoryDefaultValue,&len);
        totallen=len*2+1;
        (char *)dpd->current_value = ptp_unpack_string (sb, data, PTP_dpd_FactoryDefaultValue + totallen, &len);
        totallen+=len*2+1;
        break;
    }
    /* if totallen==0 then Data Type format is not supported by this
    code or the Data Type is a string (with two empty strings as
    values). In both cases Form Flag should be set to 0x00 and FORM is
    not present. */
    dpd->form_flag=PTP_DPFF_None;
    if (totallen==0) return;

    dpd->form_flag=dtoh8a(&data[PTP_dpd_FactoryDefaultValue+totallen]);
    totallen+=sizeof(__u8);
    switch (dpd->form_flag)
    {
    case PTP_DPFF_Range:
        switch (dpd->data_type)
        {
        case PTP_DTC_INT8:
            CTVA(int8_t,dtoh8apd,dpd->form.range.minimum_value);
            CTVA(int8_t,dtoh8apd,dpd->form.range.maximum_value);
            CTVA(int8_t,dtoh8apd,dpd->form.range.step_size);
            break;
        case PTP_DTC_UINT8:
            CTVA(__u8,dtoh8apd,dpd->form.range.minimum_value);
            CTVA(__u8,dtoh8apd,dpd->form.range.maximum_value);
            CTVA(__u8,dtoh8apd,dpd->form.range.step_size);
            break;
        case PTP_DTC_INT16:
            CTVA(int16_t,dtoh16apd,dpd->form.range.minimum_value);
            CTVA(int16_t,dtoh16apd,dpd->form.range.maximum_value);
            CTVA(int16_t,dtoh16apd,dpd->form.range.step_size);
            break;
        case PTP_DTC_UINT16:
            CTVA(__u16,dtoh16apd,dpd->form.range.minimum_value);
            CTVA(__u16,dtoh16apd,dpd->form.range.maximum_value);
            CTVA(__u16,dtoh16apd,dpd->form.range.step_size);
            break;
        case PTP_DTC_INT32:
            CTVA(int32_t,dtoh32apd,dpd->form.range.minimum_value);
            CTVA(int32_t,dtoh32apd,dpd->form.range.maximum_value);
            CTVA(int32_t,dtoh32apd,dpd->form.range.step_size);
            break;
        case PTP_DTC_UINT32:
            CTVA(__u32,dtoh32apd,dpd->form.range.minimum_value);
            CTVA(__u32,dtoh32apd,dpd->form.range.maximum_value);
            CTVA(__u32,dtoh32apd,dpd->form.range.step_size);
            break;
            /* XXX: other int types are unimplemented */
            /* XXX: int arrays are unimplemented also */
            /* XXX: does it make any sense: "a range of strings"? */
        }
        break;
    case PTP_DPFF_Enumeration:
#define N	dpd->form.menum.number_of_values
        N = dtoh16apd(sb,data,PTP_dpd_FactoryDefaultValue+totallen);
        totallen+=sizeof(__u16);
        dpd->form.menum.supported_value = kmalloc(N*sizeof(void *), GFP_KERNEL);
        switch (dpd->data_type)
        {
        case PTP_DTC_INT8:
            MCTVA(int8_t,dtoh8apd,dpd->form.menum.supported_value,N);
            break;
        case PTP_DTC_UINT8:
            MCTVA(__u8,dtoh8apd,dpd->form.menum.supported_value,N);
            break;
        case PTP_DTC_INT16:
            MCTVA(int16_t,dtoh16apd,dpd->form.menum.supported_value,N);
            break;
        case PTP_DTC_UINT16:
            MCTVA(__u16,dtoh16apd,dpd->form.menum.supported_value,N);
            break;
        case PTP_DTC_INT32:
            MCTVA(int32_t,dtoh16apd,dpd->form.menum.supported_value,N);
            break;
        case PTP_DTC_UINT32:
            MCTVA(__u32,dtoh16apd,dpd->form.menum.supported_value,N);
            break;
        case PTP_DTC_STR:
            {
                int i;
                for (i=0;i<N;i++)
                {
                    (char *)dpd->form.menum.supported_value[i]=
                    ptp_unpack_string(sb,data,PTP_dpd_FactoryDefaultValue+totallen,&len);
                    totallen+=len*2+1;
                }
            }
            break;
        }
    }
}


