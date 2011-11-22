
*************	a Small Introduction to use and create Themes for AirDC++  *****************

---------	*** Loading Theme in the client ***	-----------------------------------------------

1. Download a Theme, extract it in the Themes directory, so the .dctheme file is under \Themes\ 
 
2. Theme dropdown list can be found in colors & Fonts page in settings
	-dropdown searches for theme files ".dctheme" in the app \Themes directory
	
3. Select a theme, client will load it.
	-If the theme has icons set, client will ask if you would like to load the icons.
	-Changing icon pack will require client restart

-----------------------------------------------------------------------------------------------

---------	*** Creating a Theme for AirDC++ ***	-------------------------------------------	

Themes are stored in the client folder called Themes.
To Create a theme file, make the color settings you wish to export as a theme ( chatcolors, userlistcolors, tabcolors )
click on the button "Export Theme" in Colors & Fonts page, select the name for your theme file, and save it in the \Themes\ folder.
If you have your own Toolbars or custom icons and you want to share them with the theme you will need to edit the .dctheme theme file in a text editor.

	-Add the paths to your iconpack in the end of the .dctheme file like this:
	-Make sure the paths exist in the themes directory, otherwise the client can't load any icons!!
	<Icons>
		<IconPath type="string">Themes\Example_iconPack\icons</IconPath>
		<ToolbarImage string="string">Themes\Example_iconPack\icons\toolbar.bmp</ToolbarImage>
		<ToolbarHot type="string">Themes\Example_iconPack\icons\toolbarHot.bmp</ToolbarHot>
	</Icons>
</DCPlusPlus>

For More detailed example see example_theme.dctheme
** Example_iconpack is using icons made by spaljeni **

--------------------------------------------------------------------------------------------------