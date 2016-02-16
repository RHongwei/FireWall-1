#include "precomp.h"
#include "ntstrsafe.h"
#include "common.h"

ParamConfig *paramdev;
	int flag=0;
void Display()
{

	DBGPRINT(("==>Pt DevIoControl: %d\n", paramdev->protocol));
	DBGPRINT(("==>Pt DevIoControl: %x.%x.%x.%x\n", paramdev->src_address.field1,paramdev->src_address.field2,paramdev->src_address.field3,paramdev->src_address.field4));
	DBGPRINT(("==>Pt DevIoControl: %x.%x.%x.%x\n", paramdev->dst_address.field1,paramdev->dst_address.field2,paramdev->dst_address.field3,paramdev->dst_address.field4));
	DBGPRINT(("==>Pt DevIoControl: %d\n", paramdev->action));
	DBGPRINT(("==>Pt DevIoControl: %d\n", paramdev->alltcp));
	DBGPRINT(("==>Pt DevIoControl: %d\n", paramdev->alludp));
	DBGPRINT(("==>Pt DevIoControl: %d\n", paramdev->allicmp));
	
}

void*  UTIL_AllocateMemory(unsigned int size)
{
	PVOID ptr=NULL;
	ptr=ExAllocatePool(NonPagedPool , size);
	
	return ptr;
}
BOOLEAN UTIL_Initialize(ParamConfig *pParamConfig)
{
		
	paramdev=(struct ParamConfig *)UTIL_AllocateMemory(sizeof(ParamConfig));
	paramdev->protocol=NORULE;
	paramdev->src_address.field1=0xFF;
	paramdev->src_address.field2=0xFF;
	paramdev->src_address.field3=0xFF;
	paramdev->src_address.field4=0xFF;
	paramdev->dst_address.field1=0xFF;
	paramdev->dst_address.field2=0xFF;
	paramdev->dst_address.field3=0xFF;
	paramdev->dst_address.field4=0xFF;
	paramdev->action=PASS;
	paramdev->alltcp=PASSALL;
	paramdev->alludp=PASSALL;
	paramdev->allicmp=PASSALL;
	Display();
	return TRUE;
}


BOOLEAN UTIL_AddConfig(ParamConfig *pParamConfig)
{
	paramdev=(struct ParamConfig *)UTIL_AllocateMemory(sizeof(ParamConfig));
	paramdev->protocol=pParamConfig->protocol;
	paramdev->src_address=pParamConfig->src_address;
	paramdev->dst_address=pParamConfig->dst_address;
	paramdev->action=pParamConfig->action;
	paramdev->alltcp=pParamConfig->alltcp;
	paramdev->alludp=pParamConfig->alludp;
	paramdev->allicmp=pParamConfig->allicmp;
	DBGPRINT(("==>Pt DevIoControl: %d\n", paramdev->protocol));
	DBGPRINT(("==>Pt DevIoControl: %x.%x.%x.%x\n", paramdev->src_address.field1,paramdev->src_address.field2,paramdev->src_address.field3,paramdev->src_address.field4));
	DBGPRINT(("==>Pt DevIoControl: %x.%x.%x.%x\n", paramdev->dst_address.field1,paramdev->dst_address.field2,paramdev->dst_address.field3,paramdev->dst_address.field4));
	DBGPRINT(("==>Pt DevIoControl: %d\n", paramdev->action));
	DBGPRINT(("==>Pt DevIoControl: %d\n", paramdev->alltcp));
	DBGPRINT(("==>Pt DevIoControl: %d\n", paramdev->alludp));
	DBGPRINT(("==>Pt DevIoControl: %d\n", paramdev->allicmp));
	return TRUE;
}


typedef struct in_addr {
	union {
		struct { UCHAR s_b1,s_b2,s_b3,s_b4; } S_un_b;
		struct { USHORT s_w1,s_w2; } S_un_w;
		ULONG S_addr;
	} S_un;
} IN_ADDR, *PIN_ADDR, FAR *LPIN_ADDR;

typedef struct IP_HEADER
{
	unsigned char  VIHL;          // Version and IHL
	unsigned char  TOS;           // Type Of Service
	short          TotLen;        // Total Length
	short          ID;            // Identification
	short          FlagOff;       // Flags and Fragment Offset
	unsigned char  TTL;           // Time To Live
	unsigned char  Protocol;      // Protocol
	unsigned short Checksum;      // Checksum
	struct in_addr        iaSrc;  // Internet Address - Source
	struct in_addr        iaDst;  // Internet Address - Destination
}IP_HEADER, *PIP_HEADER;

#define IP_OFFSET                               0x0E

//IP Protocol Types
#define PROT_ICMP                               0x01 
#define PROT_TCP                                0x06 
#define PROT_UDP                                0x11 


// ���������
//	Packet�� ��������NDIS��������
//	bRecOrSend: ����ǽ��հ���ΪTRUE;���Ϊ���Ͱ���ΪFALSE��
// ����ֵ��
//	���������£�������ͨ������ֵ�Ծ�����δ���NDIS����������ʧ�ܡ�ת��
FILTER_STATUS AnalysisPacket(PNDIS_PACKET Packet,  BOOLEAN bRecOrSend)
{
	FILTER_STATUS status = STATUS_PASS; // Ĭ��ȫ��ͨ��
	PNDIS_BUFFER NdisBuffer ;
	UINT TotalPacketLength = 0;
	UINT copysize = 0;
	UINT DataOffset = 0 ;
	UINT PhysicalBufferCount;
	UINT BufferCount   ;
	PUCHAR pPacketContent = NULL;
	char* tcsPrintBuf = NULL;
	PUCHAR tembuffer = NULL ; 
	UINT j;

	while(!flag){
		UTIL_Initialize(paramdev);
		flag=1;
	}
	__try{

		status = NdisAllocateMemoryWithTag( &pPacketContent, 2048, TAG); 
		if( status != NDIS_STATUS_SUCCESS ){
			status = NDIS_STATUS_FAILURE ;
			__leave;
		}

		NdisZeroMemory( pPacketContent, 2048 ) ;

		// �ҵ���һ��Ndis_Buffer��Ȼ��ͨ��ͨ��NdisGetNextBuffer����ú�����NDIS_BUFFER��
		// ���ֻ���ҵ�һ���ڵ㣬�����ҷ���ķ����ǵ���NdisGetFirstBufferFromPacket��
		NdisQueryPacket(Packet,  // NDIS_PACKET        
			&PhysicalBufferCount,// �ڴ��е���������
			&BufferCount,		 // ���ٸ�NDIS_BUFFER��
			&NdisBuffer,         // �����ص�һ����
			&TotalPacketLength	 // �ܹ��İ����ݳ���
			);

		while(TRUE){

			// ȡ��Ndis_Buffer�д洢�������������ַ��
			// �����������һ���汾��NdisQueryBuffer��
			// ������ϵͳ��Դ�ͻ��������ľ���ʱ�򣬻����Bug Check������������
			NdisQueryBufferSafe(NdisBuffer,
				&tembuffer,// ��������ַ
				&copysize, // ��������С
				NormalPagePriority
				);

			// ���tembufferΪNULL��˵����ǰϵͳ��Դ�ѷ���
			if(tembuffer != NULL){
				NdisMoveMemory( pPacketContent + DataOffset , tembuffer, copysize) ;			
				DataOffset += copysize;
			}

			// �����һ��NDIS_BUFFER��
			// ����õ�����һ��NULLָ�룬˵���Ѿ�������ʽ��������ĩβ�����ǵ�ѭ��Ӧ�ý����ˡ�
			NdisGetNextBuffer(NdisBuffer , &NdisBuffer ) ;

			if( NdisBuffer == NULL )
				break ;
		}

		// ȡ�����ݰ����ݺ����潫�������ݽ��й��ˡ�

		if(pPacketContent[12] == 8 &&  pPacketContent[13] == 0 )  //is ip packet
		{	
			PIP_HEADER pIPHeader = (PIP_HEADER)(pPacketContent + IP_OFFSET);
			switch(pIPHeader->Protocol)
			{
			case PROT_ICMP:
				if(bRecOrSend)
				{
					DbgPrint("Tanjon Receive ICMP packet");
					if(!paramdev->allicmp)
					{
						DbgPrint("Tanjon Reject ICMP packet");
						return STATUS_DROP;
					}
					else
					{
						if(pIPHeader->iaSrc.S_un.S_un_b.s_b1==paramdev->src_address.field1 &&
							pIPHeader->iaSrc.S_un.S_un_b.s_b2==paramdev->src_address.field2 &&
							pIPHeader->iaSrc.S_un.S_un_b.s_b3==paramdev->src_address.field3 &&
							pIPHeader->iaSrc.S_un.S_un_b.s_b4==paramdev->src_address.field4 )
						{
							DbgPrint("Tanjon Reject ICMP packet");
							return STATUS_DROP;
						}	
						if(pIPHeader->iaDst.S_un.S_un_b.s_b1==paramdev->dst_address.field1 &&
							pIPHeader->iaDst.S_un.S_un_b.s_b2==paramdev->dst_address.field2 &&
							pIPHeader->iaDst.S_un.S_un_b.s_b3==paramdev->dst_address.field3 &&
							pIPHeader->iaDst.S_un.S_un_b.s_b4==paramdev->dst_address.field4 )
						{
							DbgPrint("Tanjon Reject ICMP packet");
							return STATUS_DROP;
						}
					}
				}
				else
					DbgPrint("Tanjon Send ICMP packet");
				break;

				break;
			case PROT_UDP:
				if(bRecOrSend)
				{
					DbgPrint("Tanjon receive UDP packet");
					if(paramdev->alludp==1)
					{
						DbgPrint("Tanjon block all TCP packet");
						return STATUS_DROP;
					}
					else
					{
						if(paramdev->protocol==UDP)
						{
							if(pIPHeader->iaSrc.S_un.S_un_b.s_b1==paramdev->src_address.field1 &&
							pIPHeader->iaSrc.S_un.S_un_b.s_b2==paramdev->src_address.field2 &&
							pIPHeader->iaSrc.S_un.S_un_b.s_b3==paramdev->src_address.field3 &&
							pIPHeader->iaSrc.S_un.S_un_b.s_b4==paramdev->src_address.field4 )
						{
							DbgPrint("Tanjon block UDP packet depend on src_ipaddress");
							return STATUS_DROP;
						}
						if(pIPHeader->iaDst.S_un.S_un_b.s_b1==paramdev->dst_address.field1 &&
							pIPHeader->iaDst.S_un.S_un_b.s_b2==paramdev->dst_address.field2 &&
							pIPHeader->iaDst.S_un.S_un_b.s_b3==paramdev->dst_address.field3 &&
							pIPHeader->iaDst.S_un.S_un_b.s_b4==paramdev->dst_address.field4 )
						{
							DbgPrint("Tanjon block UDP packet depend on dst_ipaddress");
							return STATUS_DROP;
						}
						}
					}
				}
				else
					DbgPrint("Tanjon send TCP packet");
				break;


			case PROT_TCP:										//�հ����ݹ��������жϣ�������������
				if(bRecOrSend)
				{
					DbgPrint("Tanjon receive TCP packet");
					if(paramdev->alltcp==DENYALLTCP)
					{
						DbgPrint("Tanjon block all TCP packet");
						return STATUS_DROP;
					}
					else
					{
						if(paramdev->protocol==TCP)
						{
							if(pIPHeader->iaSrc.S_un.S_un_b.s_b1==paramdev->src_address.field1 &&
							pIPHeader->iaSrc.S_un.S_un_b.s_b2==paramdev->src_address.field2 &&
							pIPHeader->iaSrc.S_un.S_un_b.s_b3==paramdev->src_address.field3 &&
							pIPHeader->iaSrc.S_un.S_un_b.s_b4==paramdev->src_address.field4 )
						{
							DbgPrint("Tanjon block TCP packet depend on src_ipaddress");
							DbgPrint("��ԴIP��ַ��ƥ��~�ѽ������أ�����");
							return STATUS_DROP;
						}
						if(pIPHeader->iaDst.S_un.S_un_b.s_b1==paramdev->dst_address.field1 &&
							pIPHeader->iaDst.S_un.S_un_b.s_b2==paramdev->dst_address.field2 &&
							pIPHeader->iaDst.S_un.S_un_b.s_b3==paramdev->dst_address.field3 &&
							pIPHeader->iaDst.S_un.S_un_b.s_b4==paramdev->dst_address.field4 )
						{
							DbgPrint("Tanjon block TCP packet depend on dst_ipaddress");
							DbgPrint("��Ŀ��IP��ַ��ƥ��~�ѽ������أ�����");
							return STATUS_DROP;
						}
						DBGPRINT(("��ǰIP��ַΪ�� %x.%x.%x.%x\n", pIPHeader->iaDst.S_un.S_un_b.s_b1,pIPHeader->iaDst.S_un.S_un_b.s_b2,pIPHeader->iaDst.S_un.S_un_b.s_b3,pIPHeader->iaDst.S_un.S_un_b.s_b4));
						
						}
					}
				}
				else
					DbgPrint("Tanjon send TCP packet");
				break;

			}
		}else if(pPacketContent[12] == 8 &&  pPacketContent[13] == 6 ){
			if(bRecOrSend)
				DbgPrint("Tanjon Receive ARP packet");
			else
				DbgPrint("Tanjon Send ARP packet");
		}else{
			if(bRecOrSend)
				DbgPrint("Tanjon Receive unknown packet");
			else
				DbgPrint("Tanjon Send unknown packet");
		}

		// �򵥴�ӡ������������
		status = NdisAllocateMemoryWithTag( &tcsPrintBuf, 2048*3, TAG);  //�����ڴ��
		if( status != NDIS_STATUS_SUCCESS ){
			status = NDIS_STATUS_FAILURE ;
			__leave;
		}
		for(j=0;j<=DataOffset;j++)
			RtlStringCbPrintfA(tcsPrintBuf+j*3, 2048*3-j*3, "%2x ",pPacketContent[j]);

		DbgPrint(tcsPrintBuf);

	}__finally{
		if(pPacketContent)NdisFreeMemory(pPacketContent, 0, 0);
		if(tcsPrintBuf)NdisFreeMemory(tcsPrintBuf, 0, 0);
	}

	return STATUS_PASS;
}