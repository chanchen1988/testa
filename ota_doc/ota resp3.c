typedef struct {
    uint32_t current_version;   // app1 当前版本，仅安装成功时更新
    uint32_t target_version;    // app2 待安装版本
    uint32_t image_size;        // bin 有效长度
    uint8_t  image_md5[16];     // app2 的 MD5，也是 app1 安装后应达到的 MD5

    uint8_t  pending;           // 1=存在待安装作业
    uint8_t  app1_ok;           // 1=app1 已通过安装校验，可跳转

    uint32_t retry;             // 安装重试次数，仅记录/上报
    uint32_t error;             // 最近一次错误码
    uint32_t file_source;       // network / usb
} main_ota_meta_t;


typedef struct {
    uint32_t current_version;   // 屏幕 app1 当前版本，仅安装成功时更新
    uint32_t target_version;    // 屏幕 app2 待安装版本
    uint32_t image_size;        // GUI bin 有效长度
    uint8_t  image_md5[16];     // app2 的 MD5，也是 app1 安装后应达到的 MD5

    uint8_t  pending;           // 1=屏幕 app1 已接收完并校验 app2，待 loader 安装
    uint8_t  app1_ok;           // 1=屏幕 app1 已通过安装校验，可跳转

    uint32_t retry;             // loader 安装重试次数，仅记录/上报
    uint32_t error;             // 最近一次错误码
    uint32_t file_source;       // network / usb    
} screen_ota_meta_t;


typedef struct {
    uint32_t job_id;
    uint32_t file_source;       // network / usb        

    uint32_t tx_offset;          // 已通过串口发送并 ACK 的字节数
    uint8_t  state;              // IDLE/BEGIN/DATA/END/WAIT_SCREEN_BOOT/SUCCESS/FAILED/TIMEOUT
    uint32_t error;

    uint32_t screen_current_version;
    uint8_t  screen_pending;
    uint8_t  screen_app1_ok;
    uint32_t screen_retry;
    uint32_t screen_error;
    uint32_t last_update;
} screen_ota_shadow_t;


typedef struct {
    uint32_t job_id;
    uint32_t file_source;       // network / usb        
    
    uint32_t package_version;

    uint8_t  selected_mode;      // 0=所有设备，1=单个设备
    uint16_t selected_addr;
    uint16_t total_devices;

    uint32_t lamp_addr;          // 0x90000000+0xD00000
    uint32_t lamp_size;
    uint8_t  lamp_md5[16];

    uint32_t block0_addr;        // 0x90000000+0xCFF000
    uint32_t block0_size;
    uint8_t  block0_md5[16];

    uint8_t  file_ready;         // 两文件已下载且MD5校验通过
    uint8_t  phase;              // IDLE/READY/RUNNING/DONE/FAILED/CANCELED
    uint32_t progress_permille;
    uint32_t success_count;
    uint32_t fail_count;
    uint32_t current_addr;
    uint32_t error;

    uint8_t  dev_state[OTA_DALI_MAX_DEV]; // 0未处理 1成功 2失败 3进行中
    uint8_t  dev_error[OTA_DALI_MAX_DEV];
} ota_dali_ext_t;


typedef struct {
    uint32_t job_id;
    uint32_t file_source;       // network / usb        
    
    uint32_t target_version;

    uint32_t selected_addr;

    uint32_t image_addr;         // 0x90000000+0xF00000
    uint32_t image_size;
    uint8_t  image_md5[16];    

    uint8_t  file_ready;
    uint8_t  phase;              // IDLE/READY/RUNNING/DONE/FAILED/CANCELED
    uint32_t progress_permille;
    uint32_t error;
} ota_zigbee_ext_t;


typedef struct {
    uint32_t magic;
    uint16_t version;
    uint32_t snapshot_len;
    uint32_t crc32;           // 覆盖后续快照内容

    main_ota_meta_t     main_target;
    screen_ota_meta_t   screen_target;
    ota_dali_ext_t    dali;
    ota_zigbee_ext_t  zigbee;

} ota_param_snapshot_t;



