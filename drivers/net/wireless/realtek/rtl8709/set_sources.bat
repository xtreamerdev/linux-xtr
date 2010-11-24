@echo off





::=====cmd\======
attrib -r cmd\sources
copy cmd\%SRC% cmd\sources

::=====crypto\======
attrib -r crypto\sources
copy crypto\%SRC% crypto\sources

::=====debug\======
attrib -r debug\sources
copy debug\%SRC% debug\sources

::=====eeprom\======
attrib -r eeprom\sources
copy eeprom\%SRC% eeprom\sources

::=====hal\RTL8711======
attrib -r hal\RTL8711\sources
copy hal\RTL8711\%SRC% hal\RTL8711\sources

::=====io\======
attrib -r io\sources
copy io\%SRC% io\sources

::=====ioctl\======
attrib -r ioctl\sources
copy ioctl\%SRC% ioctl\sources

::=====mlme\======
attrib -r mlme\sources
copy mlme\%SRC% mlme\sources

::=====mp\======
attrib -r mp\sources
copy mp\%SRC% mp\sources

::=====os_intf\xp\======
attrib -r os_intf\windows\sources
copy os_intf\windows\%SRC% os_intf\windows\sources


::=====pwrctrl\======
attrib -r pwrctrl\sources
copy pwrctrl\%SRC% pwrctrl\sources


::=====recv\======
attrib -r recv\sources
copy recv\%SRC% recv\sources

::=====rf\======
attrib -r rf\sources
copy rf\%SRC% rf\sources

::=====sta_mgt\======
attrib -r sta_mgt\sources
copy sta_mgt\%SRC% sta_mgt\sources

::=====xmit\======
attrib -r xmit\sources
copy xmit\%SRC% xmit\sources
