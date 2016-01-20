To create the 'all_language.nsh' file you need to do a few things first.

Remember to use the [UNICODE](https://code.google.com/p/unsis/downloads/list) version of NSIS or it will not work.
If you try to run it with the ANSI version it will halt so do not bother to try.

You also need [Polib](https://pypi.python.org/pypi/polib) - Library for python to manipulate PO files.

To get the POT file run:
```
python build_gettext_catalog_nsi.py -i AirDC_installscript.nsi -o EN_Installer.pot -p "AirDC++" -v "1.0" -l "English"
```
and you will get 'EN_Installer.pot' to upload to [Transifex](https://www.transifex.com/projects/p/airdcpp).

Download the PO files from [transifex](https://www.transifex.com/projects/p/airdcpp) (Download for use).

You will then have one file for every language. This is the Swedish file 'for_use_airdcpp_en_installerpot_sv_SE.po',
but you need to rename it to 'sv.po'. Do the same for every PO file and put them all togheter in the 'pofiles' folder.
You can read in the 'build_locale_nsh.py' what name you need to use on the PO files.

We don't want to use all the text from 'AirDC_installscript.nsi' so we use the dummy_file.nsi file instead.
To get 'all_language.nsh' run this:
```
python build_locale_nsh.py -i dummy_file.nsi -o all_language.nsh -p "pofiles/" -l "English"
```

You now have all translated strings in 'all_language.nsh'. It will be included into the AirDC_installscript.nsi when you compile it.

