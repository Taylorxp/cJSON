/***************************************************************************************************
 * FILE: configs.c
 *
 * DESCRIPTION:
 *
 **************************************************************************************************/
#include <mtd/mtd-user.h>
#include "common.h"
#include "cJSON.h"
#include "configs.h"
#include "xprint.h"

#define CONFIG_FILE_NAME "sys_para.json"
#define MTD_FILE_NAME "/dev/mtdblock1"

cJSON* cJSON_ConfigsRoot = NULL;

inline uint8_t ctou(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    else if (c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    else if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    else
    {
        return 0;
    }
}

inline uint8_t ctoh(char* arr)
{
    uint8_t h, l;

    h = ctou(arr[0]);
    l = ctou(arr[1]);

    return (h << 4) | l;
}

/* 版本号转字符串 */
char* ver2str(uint8_t* ver, uint8_t num)
{
    static char str_ver[] = "11.22.33";

    if (ver == NULL || num < 1 || num > 3)
        return NULL;

    memset(str_ver, '\0', sizeof(str_ver));

    for (uint8_t i = 0; i < num; i++)
    {
        if (i == num - 1)
            sprintf(str_ver, "%d", ver[i]);
        else
            sprintf(str_ver, "%d.", ver[i]);
    }
    return str_ver;
}

/* 字符串转版本号 */
inline int32_t str2ver(char* str, uint8_t* ver)
{
    return 0;
}

/* 字符串转MAC地址 */
uint8_t* str2mac(char* str)
{
    static uint8_t mac[6]     = {0};
    char           str_tmp[3] = "";
    char*          pstr       = NULL;

    memset(mac, 0, sizeof(mac));
    for (int32_t i = 0; i < 5; i++)
    {
        pstr = strchr(str, ':');
        memset(str_tmp, 0, sizeof(str_tmp));
        strncpy(str_tmp, str, pstr - str);
        mac[i] = ctoh(str_tmp);
        str    = pstr + 1;
    }
    pstr++;
    mac[5] = ctoh(pstr);
    return mac;
}

/* MAC地址转字符串 */
char* mac2str(uint8_t* mac)
{
    static char str_mac[18] = {0};
    memset(str_mac, '\0', sizeof(str_mac));
    sprintf(str_mac, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return str_mac;
}

/* 字符串转IP地址 */
uint32_t str2ip(char* str)
{
    uint32_t ip         = 0;
    char*    pstr       = NULL;
    char     str_tmp[3] = "";

    for (int32_t i = 0; i < 3; i++)
    {
        pstr = strchr(str, '.');
        memset(str_tmp, 0, sizeof(str_tmp));
        strncpy(str_tmp, str, pstr - str);
        ip <<= 8;
        ip |= atoi(str_tmp);
        str = pstr + 1;
    }
    pstr++;
    ip <<= 8;
    ip |= atoi(pstr);

    return ip;
}

/* IP地址转字符串 */
char* ip2str(uint32_t ip)
{
    static char str_ip[20] = {0};
    char        str_tmp[5] = {0};

    memset(str_ip, '\0', sizeof(str_ip));

    memset(str_tmp, '\0', sizeof(str_tmp));
    sprintf(str_tmp, "%d.", (ip >> 24) & 0xFF);
    strcat(str_ip, str_tmp);

    memset(str_tmp, '\0', sizeof(str_tmp));
    sprintf(str_tmp, "%d.", (ip >> 16) & 0xFF);
    strcat(str_ip, str_tmp);

    memset(str_tmp, '\0', sizeof(str_tmp));
    sprintf(str_tmp, "%d.", (ip >> 8) & 0xFF);
    strcat(str_ip, str_tmp);

    memset(str_tmp, '\0', sizeof(str_tmp));
    sprintf(str_tmp, "%d", (ip >> 0) & 0xFF);
    strcat(str_ip, str_tmp);

    return str_ip;
}

int get_file_size(char* filename)
{
    struct stat statbuf;
    stat(filename, &statbuf);
    int size = statbuf.st_size;
    return size;
}


/***************************************************************************************************
 * Description:
 **************************************************************************************************/
#define DATA_SIZE 1024 * 64
char* read_json_from_mtd(void)
{
    int32_t mtd_fd    = 0;
    char*   json_data = NULL;

    mtd_fd = open(MTD_FILE_NAME, O_RDWR);
    if (mtd_fd == 0)
    {
        PRT_ERROR("open %s failure!", MTD_FILE_NAME);
        return NULL;
    }

    // mtd_info_t mtdinfo;
    // if (ioctl(mtd_fd, MEMGETINFO, &mtdinfo) == 0)
    // {
    //     erase_info_t erase;
    //     erase.start = 0;
    //     erase.length = 64*1024*4;

    //     if (ioctl(mtd_fd, MEMERASE, &erase) != 0)
    //     {
    //         perror("\nMTD Erase failure");
    //         close(mtd_fd);
    //         return 8;
    //     }
    // }

    /*read file from this device*/
    json_data = (char*)malloc(DATA_SIZE);
    if (json_data == NULL)
    {
        close(mtd_fd);
        return NULL;
    }

    lseek(mtd_fd, 0, SEEK_SET);
    if (read(mtd_fd, json_data, DATA_SIZE) <= 0)
    {
        PRT_ERROR("read json config file failure!");
        free(json_data);
        close(mtd_fd);
        return NULL;
    }
    close(mtd_fd);
    return json_data;
}

int32_t write_json_to_mtd(char* data, int32_t size)
{
    int32_t mtd_fd = 0;

    if (size > DATA_SIZE)
    {
        PRT_ERROR("data size too large! size:%d", size);
        return -1;
    }

    mtd_fd = open(MTD_FILE_NAME, O_RDWR);
    if (mtd_fd == 0)
    {
        PRT_ERROR("open %s failure!", MTD_FILE_NAME);
        return -1;
    }
    // lseek(mtd_fd, 0, SEEK_SET); //SEEK_SET，SEEK_CUR，SEEK_END
    if (write(mtd_fd, data, size) != size)
    {
        PRT_ERROR("write json data failure!");
        close(mtd_fd);
        return -1;
    }
    close(mtd_fd);
    return 0;
}

char* read_json_from_file(void)
{
    int32_t  json_fd   = 0;
    uint32_t json_size = 0;
    char*    json_data = NULL;

    json_fd = open(CONFIG_FILE_NAME, O_RDONLY);
    if (json_fd == 0)
    {
        PRT_ERROR("open config file failure!");
        return NULL;
    }

    json_size = get_file_size("sys_para.json");
    json_data = (char*)malloc(json_size);
    if (read(json_fd, json_data, json_size) <= 0)
    {
        PRT_ERROR("read json config file failure!");
        free(json_data);
        return NULL;
    }
    return json_data;
}

int32_t write_json_to_file(char* data, int32_t size)
{
    int32_t json_fd = 0;

    json_fd = open(CONFIG_FILE_NAME, O_RDWR);
    if (json_fd == 0)
    {
        PRT_ERROR("open config file failure!");
        return -1;
    }

    ftruncate(json_fd, 0);
    lseek(json_fd, 0, SEEK_SET);
    if (write(json_fd, data, size) <= 0)
    {
        PRT_ERROR("write json config file failure!");
        return -1;
    }
    fsync(json_fd);
    close(json_fd);

    return 0;
}

/***************************************************************************************************
 * Description: 加载默认设置
 **************************************************************************************************/
cJSON* load_default_configs_json(configs_t* cfg)
{
    cJSON* root;
    cJSON* tmp;
    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "para version", "191026");
    cJSON_AddStringToObject(root, "para checksum", "0000");

    tmp = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "product info", tmp);
    cJSON_AddStringToObject(tmp, "model", "k1b");
    cJSON_AddStringToObject(tmp, "brief", "camera control unit.");
    cJSON_AddStringToObject(tmp, "serial number", "0000000000000000");

    tmp = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "system info", tmp);
    cJSON_AddStringToObject(tmp, "hardware version", "1.0");
    cJSON_AddStringToObject(tmp, "software version", "1.0.0");
    cJSON_AddStringToObject(tmp, "boot version", "1.0.0");
    cJSON_AddStringToObject(tmp, "kernel version", "1.0.0");
    cJSON_AddStringToObject(tmp, "rootfs version", "1.0.0");
    cJSON_AddStringToObject(tmp, "daemon version", "1.0.0");
    cJSON_AddStringToObject(tmp, "app version", "1.0.0");

    tmp = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "network", tmp);
    cJSON_AddStringToObject(tmp, "mac", "54:87:62:00:00:01");
    cJSON_AddStringToObject(tmp, "dhcp", "on");
    cJSON_AddStringToObject(tmp, "ip", "192.168.1.223");
    cJSON_AddStringToObject(tmp, "netmask", "255.255.255.0");
    cJSON_AddStringToObject(tmp, "gateway", "192.168.1.1");
    cJSON* dns = cJSON_CreateArray();
    cJSON_AddItemToArray(dns, cJSON_CreateString("192.168.1.1"));
    cJSON_AddItemToArray(dns, cJSON_CreateString("192.168.1.1"));
    cJSON_AddItemToObject(tmp, "dns", dns);

    return root;
}

static int32_t get_ver(const char* verstr, uint8_t* verarr, uint8_t size)
{
    int     str_len    = strlen(verstr);
    char    str_tmp[3] = "";
    int32_t pos_ver    = 0;
    int32_t pos_str    = 0;
    memset(verarr, 0, size);
    for (int32_t i = 0; i < str_len; i++)
    {
        if (verstr[i] == '.')
        {
            verarr[pos_ver] = atoi(str_tmp);
            if (++pos_ver > (size - 1))
                return 0;
            pos_str = 0;
            memset(str_tmp, 0, sizeof(str_tmp));
        }
        else
        {
            str_tmp[pos_str] = verstr[i];
            if (++pos_str > 2)
                return 0;
        }
    }
    return 1;
}

/***************************************************************************************************
 * Description: 从配置文件中读取配置
 **************************************************************************************************/
int32_t ReadConfigs(configs_t* cfg)
{
    char* json_data = NULL;

    if (cfg == NULL)
    {
        PRT_ERROR("null configs ptr!");
        return -1;
    }

    json_data = read_json_from_mtd();
    if (json_data == NULL)
    {
        PRT_ERROR("read_json_from_mtd failure!");
        return -1;
    }

    cJSON* json_tmp = NULL;
    if (cJSON_ConfigsRoot != NULL)
        cJSON_free(cJSON_ConfigsRoot);

    cJSON_ConfigsRoot = cJSON_Parse(json_data);
    // cJSON_ConfigsRoot = load_default_configs_json(cfg);
    free(json_data);
    if (cJSON_ConfigsRoot == NULL)
    {
        PRT_WARN("cJSON_Parse error!\n load default configs!");
        cJSON_ConfigsRoot = load_default_configs_json(cfg);
        if (cJSON_ConfigsRoot == NULL)
        {
            PRT_WARN("load default configs failure!");
            return -1;
        }
    }

    PRT_LOG("\nconfigs:\n%s\n", cJSON_Print(cJSON_ConfigsRoot));

    json_tmp = cJSON_GetObjectItem(cJSON_ConfigsRoot, "product info");
    if (json_tmp != NULL)
    {
        char* str = NULL;

        str = cJSON_GetStringValue(cJSON_GetObjectItem(json_tmp, "model"));
        if (str != NULL)
        {
            memset(cfg->product_info.model, 0, sizeof(cfg->product_info.model));
            strcpy(cfg->product_info.model, str);
            PRT_LOG("model:%s", cfg->product_info.model);
        }

        str = cJSON_GetStringValue(cJSON_GetObjectItem(json_tmp, "brief"));
        if (str != NULL)
        {
            memset(cfg->product_info.brief, 0, sizeof(cfg->product_info.brief));
            strcpy(cfg->product_info.brief, str);
            PRT_LOG("brief:%s", cfg->product_info.brief);
        }

        str = cJSON_GetStringValue(cJSON_GetObjectItem(json_tmp, "serial number"));
        if (str != NULL)
        {
            memset(cfg->sys_info.serial_number, 0, sizeof(cfg->sys_info.serial_number));
            strcpy(cfg->sys_info.serial_number, str);
            PRT_LOG("serial number:%s", cfg->sys_info.serial_number);
        }
    }
    else
    {
        PRT_WARN("product info node not found!");
    }

    json_tmp = cJSON_GetObjectItem(cJSON_ConfigsRoot, "system info");
    if (json_tmp != NULL)
    {
        char* str = NULL;

        str = cJSON_GetStringValue(cJSON_GetObjectItem(json_tmp, "hardware version"));
        if (str != NULL)
        {
            get_ver(str, cfg->sys_info.hw_ver, 2);
            PRT_LOG("hardware version: %d.%d", cfg->sys_info.hw_ver[0], cfg->sys_info.hw_ver[1]);
        }

        str = cJSON_GetStringValue(cJSON_GetObjectItem(json_tmp, "software version"));
        if (str != NULL)
        {
            get_ver(str, cfg->sys_info.sw_ver, 3);
            PRT_LOG("software version: %d.%d.%d", cfg->sys_info.sw_ver[0], cfg->sys_info.sw_ver[1], cfg->sys_info.sw_ver[2]);
        }

        str = cJSON_GetStringValue(cJSON_GetObjectItem(json_tmp, "mcu version"));
        if (str != NULL)
        {
            get_ver(str, cfg->sys_info.mcu_ver, 3);
            PRT_LOG("mcu version: %d.%d.%d", cfg->sys_info.mcu_ver[0], cfg->sys_info.mcu_ver[1], cfg->sys_info.mcu_ver[2]);
        }

        str = cJSON_GetStringValue(cJSON_GetObjectItem(json_tmp, "boot version"));
        if (str != NULL)
        {
            get_ver(str, cfg->sys_info.boot_ver, 3);
            PRT_LOG("boot version: %d.%d.%d", cfg->sys_info.boot_ver[0], cfg->sys_info.boot_ver[1], cfg->sys_info.boot_ver[2]);
        }

        str = cJSON_GetStringValue(cJSON_GetObjectItem(json_tmp, "kernel version"));
        if (str != NULL)
        {
            get_ver(str, cfg->sys_info.kernel_ver, 3);
            PRT_LOG("kernel version: %d.%d.%d", cfg->sys_info.kernel_ver[0], cfg->sys_info.kernel_ver[1], cfg->sys_info.kernel_ver[2]);
        }

        str = cJSON_GetStringValue(cJSON_GetObjectItem(json_tmp, "rootfs version"));
        if (str != NULL)
        {
            get_ver(str, cfg->sys_info.rootfs_ver, 3);
            PRT_LOG("rootfs version: %d.%d.%d", cfg->sys_info.rootfs_ver[0], cfg->sys_info.rootfs_ver[1], cfg->sys_info.rootfs_ver[2]);
        }

        str = cJSON_GetStringValue(cJSON_GetObjectItem(json_tmp, "daemon version"));
        if (str != NULL)
        {
            get_ver(str, cfg->sys_info.daemon_ver, 3);
            PRT_LOG("daemon version: %d.%d.%d", cfg->sys_info.daemon_ver[0], cfg->sys_info.daemon_ver[1], cfg->sys_info.daemon_ver[2]);
        }

        str = cJSON_GetStringValue(cJSON_GetObjectItem(json_tmp, "app version"));
        if (str != NULL)
        {
            get_ver(str, cfg->sys_info.app_ver, 3);
            PRT_LOG("app version: %d.%d.%d", cfg->sys_info.app_ver[0], cfg->sys_info.app_ver[1], cfg->sys_info.app_ver[2]);
        }
    }
    else
    {
        PRT_WARN("system info node not found!");
    }

    json_tmp = cJSON_GetObjectItem(cJSON_ConfigsRoot, "network");
    if (json_tmp != NULL)
    {
        cJSON* json_network = json_tmp;  // cJSON_GetArrayItem(json_tmp, 0);
        cJSON* json_dns     = NULL;

        char* str = NULL;

        /* MAC */
        str = cJSON_GetStringValue(cJSON_GetObjectItem(json_network, "mac"));
        if (str != NULL)
        {
            memcpy(cfg->net_info.mac, str2mac(str), 6);
            PRT_LOG("mac: %s", mac2str(cfg->net_info.mac));
        }

        /* DHCP */
        str = cJSON_GetStringValue(cJSON_GetObjectItem(json_network, "dhcp"));
        if (str != NULL)
        {
            if (strcmp(str, "on") == 0)
            {
                cfg->net_info.dhcp = 1;
            }
            else
            {
                cfg->net_info.dhcp = 0;
            }
            PRT_LOG("dhcp:%d", cfg->net_info.dhcp);
        }

        /* IP */
        str = cJSON_GetStringValue(cJSON_GetObjectItem(json_network, "ip"));
        if (str != NULL)
        {
            cfg->net_info.ip = str2ip(str);
            PRT_LOG("ip: %s", str);
        }

        /* MASK */
        str = cJSON_GetStringValue(cJSON_GetObjectItem(json_network, "netmask"));
        if (str != NULL)
        {
            cfg->net_info.netmask = str2ip(str);
            PRT_LOG("netmask: %s", str);
        }

        str = cJSON_GetStringValue(cJSON_GetObjectItem(json_network, "gateway"));
        if (str != NULL)
        {
            cfg->net_info.gateway = str2ip(str);
            PRT_LOG("gateway: %s", str);
        }

        json_dns = cJSON_GetArrayItem(cJSON_GetObjectItem(json_network, "dns"), 0);
        str      = cJSON_GetStringValue(json_dns);
        if (str != NULL)
        {
            cfg->net_info.dns[0] = str2ip(str);
            PRT_LOG("dns0: %s", str);
        }

        json_dns = cJSON_GetArrayItem(cJSON_GetObjectItem(json_network, "dns"), 1);
        str      = cJSON_GetStringValue(json_dns);
        if (str != NULL)
        {
            cfg->net_info.dns[1] = str2ip(str);
            PRT_LOG("dns1: %s", str);
        }
    }
    else
    {
        PRT_WARN("network node not found!");
    }

    

    return 0;
}

/***************************************************************************************************
 * Description: 配置到文件中
 **************************************************************************************************/
int32_t SaveConfigs(configs_t* cfg)
{
    char* json_data = NULL;

    if (cfg == NULL)
    {
        PRT_ERROR("null configs ptr!");
        return -1;
    }

    cJSON* json_tmp = NULL;

    if (cJSON_ConfigsRoot == NULL)
    {
        PRT_WARN("cJSON_Parse error!\n load default configs!");
        cJSON_ConfigsRoot = load_default_configs_json(cfg);
        if (cJSON_ConfigsRoot == NULL)
        {
            PRT_WARN("load default configs failure!");
            return -1;
        }
    }

    json_tmp = cJSON_GetObjectItem(cJSON_ConfigsRoot, "product info");
    if (json_tmp != NULL)
    {
        cJSON_ReplaceItemInObject(json_tmp, "model", cJSON_CreateString(cfg->product_info.model));
        cJSON_ReplaceItemInObject(json_tmp, "brief", cJSON_CreateString(cfg->product_info.brief));
        cJSON_ReplaceItemInObject(json_tmp, "serial number", cJSON_CreateString(cfg->sys_info.serial_number));
    }
    else
    {
        PRT_WARN("product info node not found!");
    }

    json_tmp = cJSON_GetObjectItem(cJSON_ConfigsRoot, "system info");
    if (json_tmp != NULL)
    {
        /* 硬件版本 */
        cJSON_ReplaceItemInObject(json_tmp, "hardware version", cJSON_CreateString(ver2str(cfg->sys_info.hw_ver, 2)));

        /* 软件版本 */
        cJSON_ReplaceItemInObject(json_tmp, "software version", cJSON_CreateString(ver2str(cfg->sys_info.sw_ver, 3)));

        /* boot版本 */
        cJSON_ReplaceItemInObject(json_tmp, "boot version", cJSON_CreateString(ver2str(cfg->sys_info.boot_ver, 3)));

        /* kernel版本 */
        cJSON_ReplaceItemInObject(json_tmp, "kernel version", cJSON_CreateString(ver2str(cfg->sys_info.kernel_ver, 3)));

        /* rootfs版本 */
        cJSON_ReplaceItemInObject(json_tmp, "rootfs version", cJSON_CreateString(ver2str(cfg->sys_info.rootfs_ver, 3)));

        /* 守护程序版本 */
        cJSON_ReplaceItemInObject(json_tmp, "daemon version", cJSON_CreateString(ver2str(cfg->sys_info.daemon_ver, 3)));

        /* APP版本 */
        cJSON_ReplaceItemInObject(json_tmp, "app version", cJSON_CreateString(ver2str(cfg->sys_info.app_ver, 3)));

        /* MCU程序版本 */
        cJSON_ReplaceItemInObject(json_tmp, "mcu version", cJSON_CreateString(ver2str(cfg->sys_info.mcu_ver, 3)));
    }
    else
    {
        PRT_WARN("system info node not found!");
    }

    json_tmp = cJSON_GetObjectItem(cJSON_ConfigsRoot, "network");
    if (json_tmp != NULL)
    {
        cJSON* json_network = json_tmp;
        cJSON* json_dns     = NULL;

        /* MAC */
        cJSON_ReplaceItemInObject(json_network, "mac", cJSON_CreateString(mac2str(cfg->net_info.mac)));

        /* DHCP */
        cJSON_ReplaceItemInObject(json_network, "dhcp", cJSON_CreateString(cfg->net_info.dhcp > 0 ? "on" : "off"));

        /* IP */
        cJSON_ReplaceItemInObject(json_network, "ip", cJSON_CreateString(ip2str(cfg->net_info.ip)));

        /* MASK */
        cJSON_ReplaceItemInObject(json_network, "netmask", cJSON_CreateString(ip2str(cfg->net_info.netmask)));

        /* Gateway */
        cJSON_ReplaceItemInObject(json_network, "gateway", cJSON_CreateString(ip2str(cfg->net_info.gateway)));

        /* DNS */
        json_dns = cJSON_GetObjectItem(json_network, "dns");
        cJSON_ReplaceItemInArray(json_dns, 0, cJSON_CreateString(ip2str(cfg->net_info.dns[0])));
        cJSON_ReplaceItemInArray(json_dns, 1, cJSON_CreateString(ip2str(cfg->net_info.dns[1])));
    }
    else
    {
        PRT_WARN("network node not found!");
    }

    PRT_LOG("configs:\n%s", cJSON_Print(cJSON_ConfigsRoot));
    json_data           = cJSON_PrintBuffered(cJSON_ConfigsRoot, 1, 1);
    int32_t json_length = strlen(json_data);
    write_json_to_mtd(json_data, json_length);
    free(json_data);

    return json_length;
}

/****************************************** END OF FILE *******************************************/