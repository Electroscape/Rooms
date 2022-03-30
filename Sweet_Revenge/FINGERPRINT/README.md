To disable the minimize/maximize buttonsin RPi

All windows will be instantly affected though it is easy to change it back.
I pretty much already gave step by step...

1. Open a terminal and type obconf then press [Enter]
2. Click on [Appearance] in left side pane.
3. Find under Window Titles..... Button order: [NLIMC]
4. Remove all except "LC" from the Button order: [LC]
5. Close obconf and ignore error messages in terminal.

Note that Openbox Config Manager (obconf) can be added to the Preferences in main menu by using main menu editor.
