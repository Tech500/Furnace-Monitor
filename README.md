"# Furnace-Monitor.ino" 

Early testing stage of sketch; furnace "ON" time is monitored using web interface, in addition to Serial Monitor.

Every time furnace runs --elapsed time is logged to "Total Minutes for Day." "Total Time for MONTH" and "Total Minutes for Year" are only
updated once when newDay function is called at a few seconds before midnight. This is being done to minimize write cycles. If DATE from
"GetDateTime" function equals days from numberDays function, then "Total Minutes for the MONTH gets reset to zero. 

Schematic file for Opto Isolator is included.  Have been using this circuit to simulate furnace runtime.  Pushbutton press starts the timing cycle, then pushbutton release ends timing cycle with display on Serial Monitor of "Accumulated Minutes on the Day."

Web interface is for Local Area Network; however with port forwarding could be visiable on the Internet.  This is not recommended due to security concerns..
