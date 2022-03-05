<?php

	define ("ADMIN_EMAIL", "");

	define ("DEBUG_MODE", false);
	define ("SOCKET_OPEN_TIMEOUT", (float) 1.5);
	define ("SOCKET_READ_TIMEOUT", (int) 2);
	
	define ("COMMENTARY_URL", "mumble://voice.quakeworld.nu");
	define ("COMMENTARY_TIMEOUT", (int) 60*60*5);
	
	// maximum failed connect attempts before server is disabled (state set to errored)
	define ("MAX_ERRORS", 30);
	
	// with this probability successfull connection will cause error counter decrease
	define ("ERROR_REDUCTION_COEF", 0.01); 
	
	define ("TOPBAR_ADDR", ROOT.'/qwnetwork/qwnetwork.php');
	define ("TOPBAR_CSS", 'http://network.quakeworld.nu/bar/style.css');
