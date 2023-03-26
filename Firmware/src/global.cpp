#include "global.h"
#include "config.h"
#include "EEPROM.h"
#include <DateTime.h>
#include <Arduino.h>
uint64_t _timestamp = 0;
long _micros = 0;
char _name[64] = {0};
char _ssid[64] = {0};
char _password[64] = {0};
void global_init()
{

    pinMode(LED_IO, OUTPUT);
    for (int i = 0; i < 3; i++)
    {
        digitalWrite(LED_IO, LOW);
        delay(100);
        digitalWrite(LED_IO, HIGH);
        delay(100);
    }
    digitalWrite(LED_IO, LOW);

    EEPROM.begin(1024);
    if (global_has_name())
    {
        for (int i = 2; i < 66; i++)
        {
            _name[i - 2] = EEPROM.read(i);
        }
    }
    else
    {
        strcpy(_name, "ONS-CAM\0");
    }
    if (global_has_ssid())
    {
        for (int i = 69; i < 133; i++)
        {
            _ssid[i - 69] = EEPROM.read(i);
            if(_ssid[i - 69]==0||_ssid[i - 69]==0xff){
                _ssid[i - 69] = 0;
                break;
            }
        }
    }
    if (global_has_password())
    {
        for (int i = 136; i < 200; i++)
        {
            _password[i - 136] = EEPROM.read(i);
            if(_password[i - 136]==0||_password[i - 136]==0xff)
            {
                _password[i - 136] = 0;
                break;
            }
        }
    }
    _micros = micros();
}
bool global_has_name()
{
    return EEPROM.read(1)=='W';
}
void global_set_name(const char *name)
{
    strcpy(_name, name);
    EEPROM.write(1, 'W');
    int i,len;
    for (i = 2, len = 66; i < len; i++)
    {
        EEPROM.write(i, _name[i - 2]);
        if (_name[i - 2] == '\0'||_name[i - 2] == '\n')
        {
            break;
        }
    }
    EEPROM.write(i, 0);
    EEPROM.commit();
}
void global_set_ssid(const char *ssid)
{
    strcpy(_ssid, ssid);
    EEPROM.write(68, 'W');
    int i,len;
    for (i = 69, len = 133; i < len; i++)
    {
        EEPROM.write(i, _ssid[i - 69]);
        if (_ssid[i - 69] == '\0'||_ssid[i - 69] == '\n')
        {
            break;
        }
    }
    EEPROM.write(i, 0);
    EEPROM.commit();
}
void global_set_password(const char *password)
{
    strcpy(_password, password);
    EEPROM.write(135, 'W');
    int i,len;
    for (i = 136, len = 200; i < len; i++)
    {
        EEPROM.write(i, _password[i - 136]);
        if (_password[i - 136] == '\0'||_password[i - 136] == '\n')
        {
            break;
        }
    }
    EEPROM.write(i, 0);
    EEPROM.commit();
}
bool global_has_ssid()
{
    return EEPROM.read(68) == 'W';
}
bool global_has_password()
{
    return EEPROM.read(135) == 'W';
}
bool IRAM_ATTR global_has_exposure()
{
    return EEPROM.read(300) == 'W';
}
uint32_t IRAM_ATTR global_get_coarse()
{
    return EEPROM.readUInt(301);
}
void IRAM_ATTR global_set_coarse(uint32_t v)
{
    EEPROM.writeUInt(301,v);
}
void IRAM_ATTR global_store_exposure()
{
    EEPROM.write(300,'W');
    EEPROM.commit();
}
uint16_t IRAM_ATTR global_get_fine()
{
    return EEPROM.readUShort(305);
}
void IRAM_ATTR global_set_fine(uint16_t v)
{
    EEPROM.writeUShort(305,v);
}
bool IRAM_ATTR global_has_gain()
{
    return EEPROM.read(307) == 'W';
}
int IRAM_ATTR global_get_r_gain()
{
    return EEPROM.read(308);
}
void IRAM_ATTR global_set_r_gain(uint8_t v)
{
    EEPROM.writeByte(308,v);
}
int IRAM_ATTR global_get_gr_gain()
{
    return EEPROM.read(309);
}
void IRAM_ATTR global_set_gr_gain(uint8_t v)
{
    EEPROM.writeByte(309,v);
}
int IRAM_ATTR global_get_gb_gain()
{
    return EEPROM.read(310);
}
void IRAM_ATTR global_set_gb_gain(uint8_t v)
{
    EEPROM.writeByte(310,v);
}
int IRAM_ATTR global_get_b_gain()
{
    return EEPROM.read(311);
}
void IRAM_ATTR global_set_b_gain(uint8_t v)
{
    EEPROM.writeByte(311,v);
}
void IRAM_ATTR global_store_gain()
{
    EEPROM.write(307,'W');
    EEPROM.commit();
}
const char *global_get_ssid()
{
    return _ssid;
}
const char *global_get_password()
{
    return _password;
}
const char *global_get_name()
{
    return _name;
}
const char *global_get_version()
{
    return "EC1.0.0";
}
void global_set_time(long time)
{
    setTime(time);
}
long global_get_time()
{
    return now();
}
void global_run()
{
}

void global_parse(char *in, char *cmd, char *data)
{
    bool cmd_start = false;
    bool data_start = false;
    int i=0;
    int cmd_idx = 0;
    int data_idx = 0;
    memset(cmd,0,128);
    memset(data,0,128);
    while(true)
    {
        char c = in[i];
        i++;
        if(c=='\n'||c=='\0'||i>127)
        {
            cmd[cmd_idx+1] = 0;
            data[data_idx+1] = 0;
            break;
        }
        if(c=='#'&&cmd_start==false&&data_start==false)
        {
            cmd_start = true;
        }
        else
        {
            if(c==':'&&cmd_start)
            {
                data_start = true;
                cmd_start = false;
            }
            else
            {
                if(cmd_start)
                {
                    cmd[cmd_idx] = c;
                    cmd_idx++;
                }
                else if(data_start)
                {
                    data[data_idx] = c;
                    data_idx++;
                }
            }
        }
    }
}
void global_reset()
{
    for(int i=0;i<1024;i++){
        EEPROM.write(i,0);
    }
    EEPROM.commit();
}