import collections
import os
import polib
from optparse import OptionParser

parser = OptionParser()
parser.add_option("-i", "--input", dest="input",
                  help="dummy_file.nsi location", default="dummy_file.nsi" )
parser.add_option("-o", "--output", dest="output",
                  help="Localized nightly output location", default="all_language.nsh")
parser.add_option("-p", "--podir", dest="podir",
                  help="Directory containing PO files", default="pofiles/")
parser.add_option("-l", "--lang", dest="lang",
                  help="Default language of the NSI", default="English" )

(options, args) = parser.parse_args()


# Define a dict to convert locale names to language names
localeToName = {
    "af-ZA" : "Afrikaans",
    "sq-AL" : "Albanian",
    "ar" : "Arabic",
    "hy" : "Armenian",
    "eu" : "Basque",
    "be" : "Belarusian",
    "bs" : "Bosnian",
    "br" : "Breton",
    "bg" : "Bulgarian",
    "ca" : "Catalan",
    "bem" : "Cibemba",
    "hr" : "Croatian",
    "cs" : "Czech",
    "da-DK" : "Danish",
    "nl-NL" : "Dutch",
    "efi" : "Efik",
    "en" : "English",
    "eo" : "Esperanto",
    "et" : "Estonian",
    "fa" : "Farsi",
    "fi-FI" : "Finnish",
    "fr-FR" : "French",
    "gl" : "Galician",
    "ka" : "Georgian",
    "de-DE" : "German",
    "el-GR" : "GreekGreece",
    "he" : "Hebrew",
    "hi" : "Hindi",
    "hu-HU" : "Hungarian",
    "is" : "Icelandic",
    "ig" : "Igbo",
    "id" : "Indonesian",
    "ga" : "Irish",
    "it-IT" : "Italian",
    "ja" : "Japanese",
    "km" : "Khmer",
    "ko" : "Korean",
    "ku" : "Kurdish",
    "lv" : "Latvian",
    "lt" : "Lithuanian",
    "lb" : "Luxembourgish",
    "mk" : "Macedonian",
    "mg" : "Malagasy",
    "ms" : "Malay",
    "mn" : "Mongolian",
    "no-NO" : "Norwegian",
    "nn" : "NorwegianNynorsk",
    "ps" : "Pashto",
    "pl-PO" : "Polish",
    "pt" : "Portuguese",
    "pt-BR" : "PortugueseBR",
    "pt-PT" : "Portuguese",
    "ro-RO" : "Romanian",
    "ru-RU" : "Russian",
    "sr" : "Serbian",
    "sr-SP" : "SerbianLatin",
    "st" : "Sesotho",
    "sn" : "Shona",
    "zh-CN" : "SimpChinese",
    "sk" : "Slovak",
    "sl" : "Slovenian",
    "es-ES" : "Spanish",
    "es-AR" : "SpanishInternational",
    "sw" : "Swahili",
    "sv-SE" : "Swedish",
    "ta" : "Tamil",
    "th" : "Thai",
    "zh-HK" : "TradChinese",
    "tr" : "Turkish",
    "tw" : "Twi",
    "uk-UA" : "Ukrainian",
    "ug" : "Uyghur",
    "uz" : "Uzbek",
    "ca@valencia" : "Valencian",
    "vi" : "Vietnamese",
    "cy" : "Welsh",
    "yo" : "Yoruba",
    "zu" : "Zulu",
}

def escapeNSIS(st):

    return st.replace('\t', r'$\\t')\
             .replace('\t', r'$\t')\
             .replace('\r', r'\r')\
             .replace('\n', r'\n')\
             .replace('\"', r'$\"')\
             .replace('$$\\', '$\\')

translationCache = {}

# The purpose of this loop is to go to the podir scanning for PO files for each locale name
# Once we've found a PO file, we use PO lib to read every translated entry
# Using this, for each each language, we store a dict of entries - { nsilabel (comment) : translation (msgstr) }
# For untranslated entries, we use msgid instead of msgstr (i.e. default English string)
for root,dirs,files in os.walk(options.podir):
    for file in files:
        filename,ext = os.path.splitext(file)
        if ext == ".po":
            # Valid locale filename (fr.po, de.po etc)?
            if filename in localeToName:
                language = localeToName[filename]
                translationCache[language] = collections.OrderedDict()
                
                # Let's add a default LANGUAGE_CODE LangString to be read
                translationCache[language]["LANGUAGE_CODE"] = filename

                po = polib.pofile(os.path.join(root,file))
                for entry in po.translated_entries():
                    # Loop through all our labels and add translation (each translation may have multiple labels)
                    for label in entry.comment.split():
                        translationCache[language][label] = escapeNSIS(entry.msgstr)
                # For untranslated strings, let's add the English entry
                for entry in po.untranslated_entries():
                    for label in entry.comment.split():
                        print("Warning: Label '%s' for language '%s' remains untranslated"%(label,language))
                        translationCache[language][label] = escapeNSIS(entry.msgid)


# Open our source NSI, dump it to a list and close it
NSISourceFile = open(options.input,"r")
NSISourceLines = NSISourceFile.readlines()
NSISourceFile.close()
NSINewLines = []

def tostr(obj):
#    if type(obj) == str:
        return obj.encode("utf-8")
#    else:
#        return obj.encode("utf-8")

# Here we scan for ";@INSERT_TRANSLATIONS@" in the NSIS, and add MUI_LANGUAGE macros and LangString's for translation languages
lineNo = 1
for line in NSISourceLines:
    if line.find(";@INSERT_TRANSLATIONS@") == 0:
        for language,translations in translationCache.items():
            count = 0
            # if the language isn't the default, we add our MUI_LANGUAGE macro
            if language.upper() != options.lang.upper():
                NSINewLines.append( tostr('!insertmacro MUI_LANGUAGE "%s"\n'%language) )
            # For every translation we grabbed from the .po, let's add our LangString
            for label,value in translations.items():
                NSINewLines.append( tostr('LangString %s ${LANG_%s} "%s"\n' % (label,language,value)) )
                count += 1
            print ( "%i translations merged for language '%s'"%(count,language) )
    else:
        NSINewLines.append ( line )
    
# Finally, let's write our new .nsi to the desired target file
NSIWorkingFile = open(options.output,"wb")
NSIWorkingFile.writelines(NSINewLines)
NSIWorkingFile.close()
    
print ( "NSH Localization Operation Complete" )
