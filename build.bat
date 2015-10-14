@set CC=cl /nologo /MT /O2 /GL /GS- /Gm- /Oi /Oy-
@set AR=lib /nologo /LTCG

@del *.obj
%CC% /Imapm mapm\*.c
%AR% /out:mapmlib.lib *.obj
@del *.obj


%CC% /LD /DNDEBUG /DLUA_BUILD_AS_DLL /ID:\Lua53\include /Imapm ^
lmapm.c D:\Lua53\lib\lua53.lib ^
mapmlib.lib /Femapm.dll

@del *.exp *.obj mapm.lib
