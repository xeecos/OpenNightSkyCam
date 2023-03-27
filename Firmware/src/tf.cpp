#include "tf.h"
#include "config.h"
#include "service.h"
#include "log.h"
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "DateTime.h"
bool _writting = false;
bool _reading = false;
#ifdef USE_MMC
#include "SD_MMC.h"
File _fileW;
File _fileR;
#define MOUNT_POINT "/sdcard"
sdmmc_card_t *_card;
#else
#include "SPI.h"
static SPIClass spi = SPIClass(FSPI);
#define SD_CONFIG SdSpiConfig(SD_D3, DEDICATED_SPI, SD_SCK_MHZ(75), &spi)
static SdExFat sd;
static ExFile _fileW;
static ExFile _fileR;
#endif

bool noSD = true;
void tf_mount()
{
#ifdef CH_EN
    pinMode(CH_EN, OUTPUT);
    digitalWrite(CH_EN, LOW);
#endif
#ifdef CH_IN
    pinMode(CH_IN, OUTPUT);
    digitalWrite(CH_IN, HIGH);
#endif

#ifdef USE_MMC
    SD_MMC.setPins(SD_CLK, SD_CMD, SD_D0, SD_D1, SD_D2, SD_D3);
    if (SD_MMC.begin(MOUNT_POINT, false, false, 40000, 5))
    {
        noSD = false;
        LOG_UART("TF Mount\n");
        // tf_test();
    }
#else
    spi.begin(SD_CLK, SD_D0, SD_CMD, SD_D3);

    delay(100);
    if (!sd.begin(SD_CONFIG))
    {
        LOG_UART("Failed to Mount\n");
        sd.initErrorHalt();
    }
    else
    {
        LOG_UART("TF Mount\n");
        noSD = false;
        // sd.format();
        // tf_test();
    }
#endif
}
void tf_reset()
{
#ifndef USE_MMC
    sd.end();
    delay(10);
    spi.end();
    spi.begin(SD_CLK, SD_D0, SD_CMD, SD_D3);
    noSD = true;
    delay(100);
    if (!sd.begin(SD_CONFIG))
    {
        LOG_UART("Failed to Mount\n");
        sd.initErrorHalt();
    }
    else
    {
        LOG_UART("TF Mount\n");
        noSD = false;
    }
#endif
}
void tf_unmount()
{
#ifdef USE_MMC
    SD_MMC.end();
#else
    sd.end();
    delay(10);
    spi.end();
    // SD_MMC.setPins(SD_CLK, SD_CMD, SD_D0, SD_D1, SD_D2, SD_D3);
    // while(!SD_MMC.begin("/sdcard", false, false, 10000))
    // {
    //     delay(1000);
    // }
    // SD_MMC.end();
#endif
#ifdef CH_EN
    pinMode(CH_EN, OUTPUT);
    digitalWrite(CH_EN, LOW);
#endif
    pinMode(CH_IN, OUTPUT);
    digitalWrite(CH_IN, LOW);
    LOG_UART("TF Unmount\n");
    noSD = true;
}
void tf_list_dir(String dirname, WebServer *server)
{
    if (noSD)
    {
        return;
    }
#ifdef USE_MMC
    File dir = dirname.equals("/") ? SD_MMC.open("/", "r") : SD_MMC.open(dirname.substring(0, dirname.length() - 1).c_str(), "r");
    if (dir.isDirectory())
    {
        server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server->sendHeader("Pragma", "no-cache");
        server->sendHeader("Expires", "-1");
        server->setContentLength(CONTENT_LENGTH_UNKNOWN);
        server->send(200, "application/json", "");
        server->sendContent(F("{\"list\":["));
        int count = 0;
        while (1)
        {
            File f = dir.openNextFile("r");
            if (f)
            {
                char *info = (char *)malloc(128);
                int size = 0;
                sprintf(info, "{\"type\":\"%s\",\"name\":\"%s\",\"size\":%d,\"time\":%d},\0", f.isDirectory() ? "dir" : "file", f.name(), f.isDirectory() ? 0 : f.size(), f.getLastWrite());
                server->sendContent(info);
                free(info);
                count++;
            }
            else
            {
                char *info = (char *)malloc(128);
                sprintf(info, "{\"count\":\"%d\"}\0", count);
                server->sendContent(info);
                free(info);
                break;
            }
            if (count > 10)
            {
                break;
            }
        }
        server->sendContent(F("]}"));
        server->sendContent(F(""));
        server->client().stop();
    }
    else
    {
        server->send(200, "text/plain", dirname.c_str());
    }
#else
    ExFile dir = dirname.equals("/") ? sd.open("/", FILE_READ) : sd.open(dirname.c_str(), FILE_READ);
    if (dir.isDirectory())
    {
        server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server->sendHeader("Pragma", "no-cache");
        server->sendHeader("Expires", "-1");
        server->setContentLength(CONTENT_LENGTH_UNKNOWN);
        server->send(200, "application/json", "");
        server->sendContent(F("{\"list\":["));
        int count = 0;
        while (1)
        {
            ExFile f = dir.openNextFile(FILE_READ);
            if (f)
            {
                char *info = (char *)malloc(128);
                int size = 0;
                char *name = (char *)malloc(64);
                f.getName(name, 64);
                uint16_t time;
                uint16_t date;
                f.getCreateDateTime(&date, &time);
                sprintf(info, "{\"type\":\"%s\",\"name\":\"%s\",\"size\":%d,\"time\":%d},\0", f.isDirectory() ? "dir" : "file", name, f.isDirectory() ? 0 : f.size(), time); //, f.getLastWrite()
                server->sendContent(info);
                free(name);
                free(info);
                count++;
            }
            else
            {
                char *info = (char *)malloc(128);
                sprintf(info, "{\"count\":\"%d\"}\0", count);
                server->sendContent(info);
                free(info);
                break;
            }
            if (count > 2048)
            {
                break;
            }
        }
        server->sendContent(F("]}"));
        server->sendContent(F(""));
        server->client().stop();
    }
    else
    {
        server->send(200, "text/plain", dirname.c_str());
    }
#endif
}

void IRAM_ATTR tf_create_dir(const char *path)
{
    if (noSD)
    {
        return;
    }
    if (!tf_exists_dir(path))
    {
#ifdef USE_MMC
        SD_MMC.mkdir(path);
#else
        // LOG_UART("mkdir:%s\n",path);
        if (path[0] == '/')
        {
            // String dir = path;
            if (!sd.mkdir(path))
            {
                // LOG_UART("mkdir fail\n");
            }
            else
            {
                // LOG_UART("mkdir success\n");
            }
        }
#endif
    }
    // else
    // {
    //     //LOG_UART("dir exist:%s\n",path);
    // }
    // else
    // {
    //     // LOG_UART("dir exists:%s\n",path);
    // }
}
bool IRAM_ATTR tf_exists(const char *path)
{
// LOG_UART("check exist\n");
#ifdef USE_MMC
    return SD_MMC.exists(path);
#else
    bool exists = sd.exists(path);
    // LOG_UART("file exists:%d\n",exists);
    return exists;
#endif
}
bool IRAM_ATTR tf_exists_dir(const char *path)
{
#ifdef USE_MMC
    return SD_MMC.exists(path);
#else
    String dir = path;
    // dir+="/";
    if (path[0] == '/')
    {
        // LOG_UART("check dir exists:%s\n",dir.substring(1,dir.length()).c_str());
        bool exists = sd.exists(dir.substring(1, dir.length()).c_str());
        // LOG_UART("dir exists:%d\n",exists);
        return exists;
    }
    else
    {
        // LOG_UART("dir:%s\n",dir.c_str());
        bool exists = sd.exists(dir.c_str());
        // LOG_UART("dir exists:%d\n",exists);
        return exists;
    }
#endif
}
bool tf_remove_dir(const char *dirname)
{
    if (noSD)
    {
        return false;
    }
    String path = dirname;
    path = path.substring(0, path.length() - 1);
#ifdef USE_MMC
    File file = SD_MMC.open(path.c_str(), "r");
    if (!file.isDirectory())
    {
        file.close();
        return SD_MMC.remove(path);
    }

    while (true)
    {
        File entry = file.openNextFile("r");
        if (!entry)
        {
            break;
        }
        String entryPath = path;
        entryPath += "/";
        entryPath += entry.name();
        if (entry.isDirectory())
        {
            entry.close();
            entryPath += "/";
            tf_remove_dir(entryPath.c_str());
        }
        else
        {
            entry.close();
            SD_MMC.remove((char *)entryPath.c_str());
        }
        yield();
    }

    SD_MMC.rmdir(path);
    file.close();
#else
    sd.remove(path);
#endif
    return true;
}
bool tf_available()
{
    return !noSD;
}
#ifdef USE_MMC
File tf_open(const char *path, const char *mode)
{
    char *dir = (char *)malloc(512);
    memset(dir, 0, 512);
    int i = 0;
    while (true)
    {
        if (path[i] == '/' && i != 0)
        {
            tf_create_dir(dir);
        }
        dir[i] = path[i];
        if (path[i] == 0)
        {
            break;
        }
        i++;
    }
    free(dir);
    if (tf_exists(path))
    {
        tf_delete_file(path);
    }
    return SD_MMC.open(path, mode, true);
}
bool tf_fulldisk()
{
    return (SD_MMC.totalBytes() - SD_MMC.usedBytes()) < (1024 * 1024 * 3);
}
#else
void tf_open(const char *path, int mode)
{

    char *dir = (char *)malloc(512);
    memset(dir, 0, 512);
    int i = 0;
    while (true)
    {
        if (path[i] == '/')
        {
            tf_create_dir(dir);
        }
        dir[i] = path[i];
        if (path[i] == 0)
        {
            break;
        }
        i++;
    }
    free(dir);
    if (tf_exists(path))
    {
        tf_delete_file(path);
    }
    _fileW.open(path, mode);
}
bool tf_fulldisk()
{
    return sd.freeClusterCount() < 500;
}
#endif
void IRAM_ATTR tf_begin_write(const char *path)
{
    // LOG_UART("path:%d %s\n",noSD, path);
    if (noSD)
    {
        return;
    }
    char *dir = (char *)malloc(255);
    memset(dir, 0, 255);
    int i = 0;
    while (true)
    {
        if (path[i] == '/')
        {
            tf_create_dir(dir);
        }
        dir[i] = path[i];
        if (path[i] == 0)
        {
            break;
        }
        i++;
    }
    free(dir);
    if (tf_exists(path))
    {
        tf_delete_file(path);
    }
#ifdef USE_MMC
    _fileW = SD_MMC.open(path, "w+");
#else
    String filepath = path;
    if (path[0] == '/')
    {
        _fileW.open(filepath.substring(1, filepath.length()).c_str(), O_WRITE | O_CREAT | O_AT_END);
    }
    else
    {
        _fileW.open(path, O_WRITE | O_CREAT | O_AT_END);
    }
#endif
    if (!_fileW)
    {
        _writting = false;
        LOG_UART("Writing err: %s\n", path);
        _fileW.close();
        delay(50);
        tf_reset();
        delay(50);
        tf_delete_file(path);
    }
    else
    {
        _writting = true;
        LOG_UART("Writing file: %s\n", path);
    }
}
void tf_write_file(uint8_t *data, size_t size, int blocksize)
{
    if (noSD)
    {
        return;
    }
    if (_writting)
    {
        // _fileW.setBufferSize(blocksize);
        if (size < blocksize || blocksize == 0)
        {
            _fileW.write(data, size);
        }
        else
        {
            int bufsize = blocksize; // 32768;//65536;
            for (int i = 0; i < size; i += bufsize)
            {
                if (i < size - bufsize)
                {
                    _fileW.write(data + i, bufsize);
                }
                else
                {
                    _fileW.write(data + i, size - i);
                }
            }
        }
        // LOG_UART("File written:%d\n", size);
    }
    else
    {
        // LOG_UART("Write failed\n");
    }
}
void tf_end_write()
{
    if (noSD)
    {
        return;
    }
    if (_writting)
    {
#ifndef USE_MMC
        _fileW.sync();
#endif
        _fileW.close();
    }
    _writting = false;
}
void tf_append_file(const char *path, const char *message)
{
    if (noSD)
    {
        return;
    }
// LOG_UART("Appending to file: %s\n", path);
#ifdef USE_MMC
    _fileW = SD_MMC.open(path, "a");
#else
    _fileW.open(path, O_RDWR | O_CREAT | O_AT_END);
#endif
    if (!_fileW)
    {
        // LOG_UART("Failed to open file for appending\n");
        return;
    }
    // _fileW.print(message);
}

void tf_rename_file(const char *path1, const char *path2)
{
    if (noSD)
    {
        return;
    }
// LOG_UART("Renaming file %s to %s\n", path1, path2);
#ifdef USE_MMC
    if (SD_MMC.rename(path1, path2))
    {
        // LOG_UART("SdFile renamed\n");
    }
    else
    {
        // LOG_UART("Rename failed\n");
    }
#endif
}
bool tf_is_directory(const char *path)
{
#ifdef USE_MMC
    File f = SD_MMC.open(path, "r");
    bool isDirectory = f.isDirectory();
    f.close();
#else
    _fileR.open(path, O_READ);
    bool isDirectory = _fileR.isDirectory();
    _fileR.close();
#endif
    return isDirectory;
}
bool tf_delete_file(const char *path)
{
    if (noSD)
    {
        return false;
    }
    bool isDir = tf_is_directory(path);
    if (isDir)
    {
        // LOG_UART("Deleting dir: %s\n", path);
        return tf_remove_dir(path);
    }
// LOG_UART("Deleting file: %s\n", path);
#ifdef USE_MMC
    return SD_MMC.remove(path);
#else
    // LOG_UART("remove file\n");
    return sd.remove(path);
#endif
}
bool tf_test()
{
    bool fast = false;
    uint8_t *buf = (uint8_t *)ps_malloc(1024 * 1024);
    for (int j = 0; j < 1; j++)
    {
        for (int i = 0; i < 3; i++)
        {
            LOG_UART("start block size:%d\n", 512 << i);
            long time = millis();
            tf_begin_write("/images/tmp.txt");
            tf_write_file(buf, 1024 * 1024, 512 << i);
            tf_end_write();
            long t = (millis() - time);
            // fast =  t < 200 && t>0;
            LOG_UART("end time:%d ms\n", t);
            tf_delete_file("/images/tmp.txt");
            // LOG_UART("start block size:%d\n",500*(1<<i));
            // time = millis();
            // tf_begin_write("/preview/202301/tmp.txt");
            // tf_write_file(buf,1024*1024,500*(1<<i));
            // tf_end_write();
            // t = (millis() - time);
            // // fast =  t < 200 && t>0;
            // LOG_UART("end time:%d ms\n",t);
            // tf_delete_file("/preview/202301/tmp.txt");
        }
    }
    free(buf);
    return fast;
}
#ifdef USE_MMC
File tf_read(const char *path)
{
    return SD_MMC.open(path, FILE_READ);
}
#else
ExFile tf_read(const char *path)
{
    _fileR.open(path, O_RDWR);
    return _fileR;
}
#endif