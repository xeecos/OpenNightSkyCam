'use strict';

window.Translator = {
    _words: [],

    translate: function (group) {
        if(!group)
        {
            group = "en";
        }
        var $this = this;
        $('[data-sw-translate]').each(function () {
            $(this).html($this._tryTranslate(group, $(this).html()));
            $(this).val($this._tryTranslate(group, $(this).val()));
            $(this).attr('title', $this._tryTranslate(group, $(this).attr('title')));
        });
    },

    _tryTranslate: function (group, word) {
        return this._words[group][$.trim(word)] !== undefined ? this._words[group][$.trim(word)] : word;
    },

    learn: function (wordsMap) {
        this._words = wordsMap;
    }
};


/* jshint quotmark: double */
window.Translator.learn(
    {
        "en": {
            "loading...":"loading...",
            "Preview": "Preview",
            "Task": "Task",
            "Timelapse": "Timelapse",
            "Browser": "Browser",
            "Setting": "Setting",
            "UDisk-Mode:": "UDisk Mode:",
            "Language:": "Language:",
            "Shot": "Shot",
            "Reload": "Reload",
            "Exposure:": "Exposure:",
            "R Gain:": "R Gain:",
            "G Gain:": "G Gain:",
            "B Gain:": "B Gain:",
            "Global Gain:":"Global Gain:",
            "Sec": "Sec",
            "Progress:": "Progress:",
            "Interval:": "Interval:",
            "Frames:": "Frames:",
            "Video Length:": "Video Length:",
            "Start Task": "Start Task",
            "Stop Task": "Stop Task",
            "Update Status": "Update Status"
        },
        "zh_CN": {
            "loading...":"加载中...",
            "Preview": "预览",
            "Task": "任务",
            "Timelapse": "延时摄影",
            "Browser": "浏览",
            "Setting": "设置",
            "UDisk-Mode:": "U盘模式:",
            "Language:": "语言:",
            "Shot": "拍摄",
            "Reload": "显示最新照片",
            "Exposure:": "曝光时间:",
            "R Gain:": "增益R:",
            "G Gain:": "增益G:",
            "B Gain:": "增益B:",
            "Global Gain:":"全局增益:",
            "Sec": "秒",
            "Progress:": "进度:",
            "Interval:": "间隔时间:",
            "Frames:": "帧数:",
            "Video Length:": "视频长度:",
            "Start Task": "开始任务",
            "Stop Task": "结束任务",
            "Update Status": "更新状态"
        }
    });

