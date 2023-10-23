// CAIDA数据集pcap文件不含以太网帧头
#include <math.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// #include<pcap.h>

typedef int32_t bpf_int32;
typedef u_int32_t bpf_u_int32;
typedef u_int16_t u_short;
typedef u_int32_t u_int32;
typedef u_int16_t u_int16;
typedef u_int8_t u_int8;

// pcap文件头结构体
struct pcap_file_header {
  bpf_u_int32 magic;     /* 0xa1b2c3d4 */
  u_short version_major; /* magjor Version 2 */
  u_short version_minor; /* magjor Version 4 */
  bpf_int32 thiszone;    /* gmt to local correction */
  bpf_u_int32 sigfigs;   /* accuracy of timestamps */
  bpf_u_int32 snaplen;   /* max length saved portion of each pkt */
  bpf_u_int32 linktype;  /* data link type (LINKTYPE_*) */
};

//时间戳
struct time_val {
  int tv_sec;  /* seconds 含义同 time_t 对象的值 */
  int tv_usec; /* and microseconds */
};

// pcap数据包头结构体
struct pcap_pkthdr {
  struct time_val ts; /* time stamp */
  bpf_u_int32 caplen; /* 当前数据区的长度，即抓取到的数据帧长度 */
  bpf_u_int32
      len; /* 离线数据长度：网络中实际数据帧的长度，一般不大于caplen，多数情况下和Caplen数值相等*/
};

//数据帧头
typedef struct FramHeader_t { // Pcap捕获的数据帧头
  u_int8 DstMAC[6];           //目的MAC地址
  u_int8 SrcMAC[6];           //源MAC地址
  u_short FrameType;          //帧类型
} FramHeader_t;

// IP数据报头(ipv4)
typedef struct IPHeader_t { // IP数据报头
  u_int8 Ver_HLen;          //版本+报头长度
  u_int8 TOS;               //服务类型
  u_int16 TotalLen;         //总长度
  u_int16 ID;               //标识
  u_int16 Flag_Segment;     //标志+片偏移
  u_int8 TTL;               //生存周期
  u_int8 Protocol;          //协议类型
  u_int16 Checksum;         //头部校验和
  u_int32 SrcIP;            //源IP地址
  u_int32 DstIP;            //目的IP地址
} IPHeader_t;

// TCP数据报头
typedef struct TCPHeader_t { // TCP数据报头
  u_int16 SrcPort;           //源端口
  u_int16 DstPort;           //目的端口
  u_int32 SeqNO;             //序号
  u_int32 AckNO;             //确认号
  u_int8 HeaderLen;          //数据报头的长度(4 bit) + 保留(4 bit)
  u_int8 Flags;              //标识TCP不同的控制消息
  u_int16 Window;            //窗口大小
  u_int16 Checksum;          //校验和
  u_int16 UrgentPointer;     //紧急指针
} TCPHeader_t;

// UDP数据
typedef struct UDPHeader_s {
  u_int16_t SrcPort;  // 源端口号16bit
  u_int16_t DstPort;  // 目的端口号16bit
  u_int16_t len;      // 数据包长度16bit
  u_int16_t checkSum; // 校验和16bit
} UDPHeader_t;

typedef struct Quintet {
  double timestamp;  //这个包的精确时间戳
  u_int32 SrcIP;     //源IP地址
  u_int32 DstIP;     //目的IP地址
  u_int16_t SrcPort; // 源端口号16bit
  u_int16_t DstPort; // 目的端口号16bit
  u_int8 Protocol;   //协议类型
  u_int16_t TotalLen;
} Quintet_t;

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("需要输入3个参数：pcap源文件, times源文件, 目标输出文件\n");
    return 0;
  }
  /* *************************************** */

  struct pcap_pkthdr *pkt_header = NULL;
  FramHeader_t *eth_header = NULL;
  IPHeader_t *ip_header = NULL;
  TCPHeader_t *tcp_header = NULL;
  UDPHeader_t *udp_header = NULL;
  Quintet_t *quintet = NULL;
  // char buf[BUFSIZE];

  //初始化
  pkt_header = (struct pcap_pkthdr *)malloc(sizeof(struct pcap_pkthdr));
  eth_header = (FramHeader_t *)malloc(sizeof(FramHeader_t));
  ip_header = (IPHeader_t *)malloc(sizeof(IPHeader_t));
  tcp_header = (TCPHeader_t *)malloc(sizeof(TCPHeader_t));
  udp_header = (UDPHeader_t *)malloc(sizeof(UDPHeader_t));
  quintet = (Quintet_t *)malloc(sizeof(Quintet_t));
  // memset(buf, 0, sizeof(buf));

  FILE *pFile = fopen(argv[1], "r");
  if (pFile == 0) {
    printf("打开pcap文件失败\n");
    return 0;
  }

  FILE *tFile = fopen(argv[2], "r");
  if (tFile == 0) {
    printf("打开times文件失败\n");
    ;
    return 0;
  }
  int timestmap_cnt = 0;

  FILE *output = fopen(argv[3], "wb");
  if (output == 0) {
    printf("打开dat文件失败\n");
    return 0;
  }

  //开始读数据包
  printf("开始读数据包\n");
  long int pkt_offset; //用来文件偏移
  pkt_offset = 24;     // pcap文件头结构 24个字节
  int i = 0;
  int j = 0;
  int k = 0;
  int l = 0;
  u_short ftype = 0;
  u_int8 ip_ver = 0;

  long tcp_cnt = 0, udp_cnt = 0;

  while (fseek(pFile, pkt_offset, SEEK_SET) == 0) //遍历数据包
  {
    i++;
    memset(pkt_header, 0, sizeof(struct pcap_pkthdr));
    memset(quintet, 0, sizeof(struct Quintet));
    // pcap_pkt_header 16 byte
    if (fread(pkt_header, 16, 1, pFile) != 1) //读pcap数据包头结构
    {
      printf("%d: can not read pkt_header\n", i);
      break;
    }

    pkt_offset += 16 + pkt_header->caplen; //下一个数据包的偏移值
    // IP数据报头 20字节 不考虑>20字节
    memset(ip_header, 0, sizeof(IPHeader_t));
    if (fread(ip_header, sizeof(IPHeader_t), 1, pFile) != 1) {
      printf("%d: can not read ip_header\n", i);
      break;
    }
    /**ipv4的ip头前4位为版本号：4，ipv6的为6**/
    ip_ver = (ip_header->Ver_HLen >> 4) & (0b00001111);
    if (ip_ver == 4) // ipv4
    {
      j++;
      double timestamp = 0;
      while (timestmap_cnt < i) {
        if (fscanf(tFile, "%lf\n", &timestamp) == EOF) {
          printf("something wrong with the times file\n");
          exit(1);
        }
        timestmap_cnt++;
      }
      quintet->timestamp = timestamp;
      quintet->SrcIP = ip_header->SrcIP;
      quintet->DstIP = ip_header->DstIP;
      quintet->TotalLen = ip_header->TotalLen;

      if (ip_header->Protocol == 0x06) {
        memset(tcp_header, 0, sizeof(tcp_header));
        if (fread(tcp_header, sizeof(tcp_header), 1, pFile) == 1) {
          tcp_cnt++;
          quintet->SrcPort = tcp_header->SrcPort;
          quintet->DstPort = tcp_header->DstPort;
          quintet->Protocol = 0x06;
          fwrite(&quintet->SrcIP, 4, 1, output);
          fwrite(&quintet->DstIP, 4, 1, output);
          fwrite(&quintet->SrcPort, 2, 1, output);
          fwrite(&quintet->DstPort, 2, 1, output);
          fwrite(&quintet->Protocol, 1, 1, output);
          fwrite(&quintet->TotalLen, 2, 1, output);
          fwrite(&quintet->timestamp, 8, 1, output);
        }
      }
      if (ip_header->Protocol == 0x11) {
        memset(udp_header, 0, sizeof(udp_header));
        if (fread(udp_header, sizeof(udp_header), 1, pFile) == 1) {
          udp_cnt++;
          quintet->SrcPort = udp_header->SrcPort;
          quintet->DstPort = udp_header->DstPort;
          quintet->Protocol = 0x11;
          fwrite(&quintet->SrcIP, 4, 1, output);
          fwrite(&quintet->DstIP, 4, 1, output);
          fwrite(&quintet->SrcPort, 2, 1, output);
          fwrite(&quintet->DstPort, 2, 1, output);
          fwrite(&quintet->Protocol, 1, 1, output);
          fwrite(&quintet->TotalLen, 2, 1, output);
          fwrite(&quintet->timestamp, 8, 1, output);
        }
      }
    } else if (ip_ver == 6) // ipv6
    {
      k++;
    } else {
      l++;
    }
  } // end while
  fclose(pFile);
  fclose(output);
  printf("Finish!\n");
  printf("ipv4 pkts: %d\n"
         "ipv6 pkts: %d\n",
         j, k);
  printf("non-IP pkts: %d\n", l);
  printf("IPv4-TCP pkts: %ld\n", tcp_cnt);
  printf("IPv4-UDP pkts: %ld\n", udp_cnt);
  printf("The output file should be %ld bytes\n", (tcp_cnt + udp_cnt) * 21);
  return 0;
}
