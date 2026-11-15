#define OTA_META_MAGIC      0x4F544150u   // "OTAP"
#define OTA_META_VER        0x0001
#define OTA_TARGET_MAX      4
#define OTA_DALI_MAX_DEV    64

typedef enum {
    OTA_TARGET_MAIN = 0,
    OTA_TARGET_SCREEN,
    OTA_TARGET_DALI,
    OTA_TARGET_ZIGBEE
} ota_target_t;

typedef enum {
    OTA_IDLE = 0,
    OTA_CHECKED,          // 已检测到新版本
    OTA_DOWNLOADING,
    OTA_DOWNLOADED,
    OTA_VERIFIED,         // md5 校验通过
    OTA_WAIT_INSTALL,     // 等待 loader / 外设进程处理
    OTA_INSTALLING,
    OTA_SUCCESS,
    OTA_FAILED,
    OTA_CANCELED
} ota_state_t;

typedef struct {
    uint8_t  target;          // ota_target_t
    uint8_t  state;           // ota_state_t
    uint8_t  pending;         // 1=待 loader/外设进程处理
    uint8_t  active_slot;     // 主/屏：APP1
    uint8_t  staging_slot;    // 主/屏：APP2
    uint8_t  loader_action;   // 1=APP2 拷贝到 APP1
    uint8_t  verified;        // md5 已通过
    uint8_t  update_source;   // HTTP/U盘/串口/主MCU推送

    uint32_t job_id;
    uint32_t current_version;
    uint32_t target_version;
    uint32_t image_addr;      // 暂存区/app2/外设文件地址
    uint32_t image_size;
    uint32_t image_max_size;  // 分区上限
    uint8_t  image_md5[16];
    char     image_name[32];

    uint32_t downloaded_bytes;
    uint32_t progress_permille; // 0~1000
    uint32_t error_code;
    uint32_t retry_count;
    uint32_t update_time;
} ota_target_meta_t;

typedef struct {
    uint8_t  enabled;
    uint8_t  selected_mode;   // 0=所有设备，1=单个设备
    uint16_t selected_addr;   // DALI 短地址

    uint32_t package_version;
    uint32_t lamp_addr;       // 0x90000000+0xD00000
    uint32_t lamp_size;
    uint8_t  lamp_md5[16];

    uint32_t block0_addr;     // 0x90000000+0xCFF000
    uint32_t block0_size;
    uint8_t  block0_md5[16];

    uint32_t total_devices;
    uint32_t success_count;
    uint32_t fail_count;
    uint32_t current_addr;
    uint32_t progress_permille;
    uint32_t state;           // ota_state_t
    uint32_t error_code;
    uint32_t job_id;

    uint8_t  dev_state[OTA_DALI_MAX_DEV]; // 0=未处理 1=成功 2=失败 3=进行中
    uint8_t  dev_error[OTA_DALI_MAX_DEV];
} ota_dali_ext_t;

typedef struct {
    uint8_t  enabled;
    uint8_t  state;
    uint32_t version;
    uint32_t image_addr;      // 0x90000000+0xF00000
    uint32_t image_size;
    uint8_t  image_md5[16];
    uint32_t progress_permille;
    uint32_t error_code;
    uint32_t job_id;
    uint32_t module_addr;     // 如 Zigbee 模块短地址/网络地址，按现有协议填
} ota_zigbee_ext_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t header_len;
    uint32_t seq;             // 单调递增
    uint32_t snapshot_len;
    uint32_t crc32;           // 覆盖后续快照内容

    uint32_t target_bitmap;
    ota_target_meta_t target[OTA_TARGET_MAX];
    ota_dali_ext_t    dali;
    ota_zigbee_ext_t  zigbee;

    uint32_t tail_crc;
} ota_param_snapshot_t;

