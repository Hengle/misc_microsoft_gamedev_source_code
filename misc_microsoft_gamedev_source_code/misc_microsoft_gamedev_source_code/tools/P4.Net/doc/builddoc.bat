@echo off
rd /s /q ndoc
mkdir ndoc

rem run ndoc
..\ext\ndoc\NDocConsole.exe -project=p4.net.ndoc

rem, overwrite custom files
copy /y ndoc-mod\contents.html ndoc\contents.html
copy /y ndoc-mod\P4.Net.hhc ndoc\P4.Net.hhc
copy /y ndoc-mod\tree.js ndoc\tree.js

rem manually run hhc... hard-coded path (sorry)
"C:\Program Files\HTML Help Workshop\hhc.exe" ndoc\P4.Net.hhp

attrib -r html\*
attrib -r chm\*

rd /s /q html
xcopy /eciyz /exclude:html.exclude ndoc\* html

copy /y ndoc\P4.Net.chm chm\P4.Net.chm