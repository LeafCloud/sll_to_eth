#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <pcap-stdinc.h>
#endif

#include <pcap.h>
#include <vector>
#include <set>
#include <map>
#include <string.h>

#define DLT_SLL 113
#define DLT_ETH 1
#define SRC_MAC {0x11,0x11,0x11,0x11,0x11,0x11}		//源MAC
#define DST_MAC {0x22,0x22,0x22,0x22,0x11,0x11}		//目的MAC

const char src_mac[6] = SRC_MAC;
const char dst_mac[6] = DST_MAC;

int sll2eth(char *pcapname, char *dstfile="out.pcap")
{
	pcap_t * rdpcap, *_wtpcap;//打开pcap的指针
	pcap_dumper_t * wtpcap_dump; 	//写pcap的指针
	char errBUF[4096] = { 0 };
	rdpcap = pcap_open_offline(pcapname,errBUF);
	if (rdpcap == NULL)
	{
		printf("Error when open pcap file. error is :%s\n", errBUF);
		return -1;
	}

	//检测是否为SLL链路

	if (pcap_datalink(rdpcap) != DLT_SLL)
	{
		printf("Warning: pcapfile [%s] is not sll file.\n", pcapname);
		printf("Warning: pcapfile datalinktype is %d\n", pcap_datalink(rdpcap));
		return 0;
	}

	_wtpcap = pcap_open_dead(DLT_ETH, 65535);//第一个参数是 linktype,以太网的linktype是1;

	wtpcap_dump = pcap_dump_open(_wtpcap, dstfile);
	
	//检查是否打开成功
	if (_wtpcap == NULL || wtpcap_dump == NULL)
	{
		printf("Error when create dst file:%s\n", dstfile);
		return -2;
	}

	int nb_transfer = 0;
	while (1)
	{
		pcap_pkthdr pktheader;
		const u_char *pktdata = pcap_next(rdpcap, &pktheader);
		if (pktdata != NULL)
			//pcap存在pcapket未处理
		{
			pcap_pkthdr new_pkthdr = pktheader;
			u_char new_data[1600] = { 0};
			int offset = 16;//SLL有16个字节的头
			int last_offset = offset;
			
			//修改长度
			new_pkthdr.caplen = pktheader.len - last_offset;
			new_pkthdr.len = pktheader.len - last_offset;
			
			//构造以太网头
			memcpy(new_data, src_mac, 6);
			memcpy(new_data + 6, dst_mac, 6);
			//填充上层协议头,只填充IPv4协议。
			new_data[12] = 0x08;
			new_data[13] = 0x00;

			//拷贝后续packet内容
			memcpy(new_data + 14, pktdata + last_offset, new_pkthdr.len);
			//检查是否为IPv4数据包（0x0800表示IPv4协议）
			if (pktdata[last_offset] == 0x08 && pktdata[last_offset + 1] == 0x00) {
    				//检查是否为TCP协议（0x06表示TCP协议）
    				if (pktdata[last_offset + 9] == 0x06) {
        					//将上层协议类型设置为IPv4（0x0800）
        					new_data[12] = 0x08;
        					new_data[13] = 0x00;
   				 }
				}


			//把修改的以太网帧写入
			new_pkthdr.caplen += 6+6+2;
			new_pkthdr.len += 6+6+2;
			pcap_dump((u_char*) wtpcap_dump, &new_pkthdr, new_data);

			nb_transfer++;
			if (nb_transfer % 10 == 0)
			{
				pcap_dump_flush(wtpcap_dump);
			}

		}
		else
		{
			pcap_dump_flush(wtpcap_dump);
			break;
		}
	}
	if (wtpcap_dump)
	{
		pcap_dump_close(wtpcap_dump);
	}
	if (rdpcap)
	{
		pcap_close(rdpcap);
	}
	if (_wtpcap)
	{
		pcap_close(_wtpcap);
	}

	return nb_transfer;
}
int main(int argc,char *argv[])
{
	if (argc != 3)
	{
		printf("Convert SLL Frame to normal Ethernet Frame.\n");
		printf("Usage:\n\t sll2eth src_pcapname dst_pcapname \n"\
				"\t\t src_pcapname: The src sll pcap to be converted.\n"\
				"\t\t dst_pcapname: The saved destination filename\n\n");
		exit(-1);
	}
	
	char * pcapname = argv[1];
	char * dstpcapname = argv[2];
	if (sll2eth(pcapname,dstpcapname) <0 )
	{
			printf("Error!!!!%s\n", pcapname);
	}

	//system("pause");
	return 0;
}