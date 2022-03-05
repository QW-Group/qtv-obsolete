<?php

	define('ROOT', '.');

	require ROOT."/conf/config.php";
	require ROOT."/inc/model.php";
	require ROOT."/inc/nocache.php";
	
	function main()
	{
		disableCacheHeaders();
		
		$model = new ServerList;
		$model->checkErroredServers();
	}
	
	main();
