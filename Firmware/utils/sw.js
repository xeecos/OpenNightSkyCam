'use strict';
 
window.Translator = {
    _words: [],
 
    translate: function () {
        var $this = this;
        $('[data-sw-translate]').each(function () {
            $(this).html($this._tryTranslate($(this).html()));
            $(this).val($this._tryTranslate($(this).val()));
            $(this).attr('title', $this._tryTranslate($(this).attr('title')));
        });
    },
 
    _tryTranslate: function (word) {
        return this._words[$.trim(word)] !== undefined ? this._words[$.trim(word)] : word;
    },
 
    learn: function (wordsMap) {
        this._words = wordsMap;
    }
};
 
 
/* jshint quotmark: double */
window.Translator.learn({
    "Preview": "预览",
    "Task": "任务",
    "Browser": "浏览",
    "Shot": "拍摄",
    "Reload": "显示最新照片",
    "Exposure:":"曝光时间:",
    "R Gain:":"增益R:",
    "G Gain:":"增益G:",
    "B Gain:":"增益B:",
    "Sec":"秒",
    "Progress:":"进度:",
    "Interval:":"间隔时间:",
    "Frames:":"帧数:",
    "Video Length:":"视频长度:",
    "Start Task":"开始任务",
    "Stop Task":"结束任务"
});
 
 
$(function () {
    window.Translator.translate();
});