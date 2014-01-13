/*
** Module   :MKVER.CMD
** Abstract :
**
** Copyright (C) Sergey I. Yevtushenko
**
** Log: Sat  06/12/2003 Created
**
*/

fin='npdjvu.h'
fin2='npdjvu.tpl'
fout='npdjvu.def'
pattern='@version@'

d0='0'
d1='0'
d2='0'
pfx=''
sfx=''

do while lines(fin) > 0
    str=linein(fin)
    parse value str with def varname varvalue tail

    if strip(def) = '#define' & strip(varvalue)<>'' then
	do
        varvalue=strip(varvalue)
        parse value varname with prefix'_'suffix rest

        if prefix \= 'VER' then
            iterate;

        if suffix='MAJOR' then
            d0=varvalue;
        if suffix='MINOR' then
            d1=varvalue;
        if suffix='BUILD' then
            d2=varvalue;

        if suffix='PREFIX' | suffix='SUFFIX' then
        do
            parse value str with def varname varvalue
            varvalue=strip(varvalue)

            if suffix='PREFIX' then
                pfx=strip(varvalue,,'"');
	        if suffix='SUFFIX' then
    	        sfx=strip(varvalue,,'"');
	    end
    end
end

call stream fin, 'c', 'close'

'@del 'fout' >nul 2>nul'

version='"'pfx'Version 'd0'.'d1'.'d2''sfx'"'

do while lines(fin2) > 0
    str = linein(fin2)

    st = pos(pattern, str)

    if st > 0 then
    do
        head=substr(str,1, st - 1)
        tail=substr(str,st + length(pattern))
/*
        say '<'head'>'
        say '<'tail'>'
*/
        str=head||version||tail
    end
    call lineout fout, str
end

