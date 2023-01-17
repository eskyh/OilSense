call "C:\Program Files\nodejs\nodevars.bat"
call inliner main.html > index.html
call gzipper c --gzip --gzip-level 9 index.html
copy /y index.html.gz D:\IoT\projects\gauge\data\index.html
