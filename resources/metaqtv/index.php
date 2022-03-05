<?php

	define ('ROOT', '.');
	
	// disable caching, from PHP manual
	header("Content-type: text/html; charset=utf-8");

	require ROOT."/conf/config.php";
	require ROOT."/inc/model.php";
	require ROOT."/inc/nocache.php";
	
	disableCacheHeaders();
	
	if (DEBUG_MODE) {
		error_reporting(E_ALL);
	}
	else {
		error_reporting(0);
	}

	define ("STREAM_ID", 0);
	define ("STREAM_URL", 1);
	
	$model = new ServerList;
	$qtvlist = $model->getList();
	
	if (isset($_GET["full"])) {
		$request_type = "htmlfull";
		$requested_page = "/nowplaying";
		$brief_listing = false;
	}
	else if (isset($_GET["rss"])) {
		$request_type = "rss";
		$requested_page = "/rss";
		if (isset($_GET["limit"])) {
			$observers_limit = (int) $_GET["limit"];
		}
		else {
			$observers_limit = 0;
		}
	}
	else {
		$requested_page = "/nowplaying";
		$request_type = "htmlactive";
		$brief_listing = true;
	}
	
	// parsing state
	$cururl = "";
	$intable = false;
	$tablelev = 0;
	$ignoreline = false;
	$errors = "";
		
	// outpu buffers
	$output_arr = array();
	$output_buf = "";
	$output_val = "";
	
	// when not false we are inside an element which contains number of observers info
	$inobservers = false;
	
	// type of server line
	$line_type = false;
	
	// for time logging, performance monitoring
	$timelog = "";
	$lastlogtime = 0;
	
	function run_time()
	{
		static $clockstart = 0;
		
		if ($clockstart == 0) {
			$clockstart = microtime(true);
			return 0;
		}
		else {
			return microtime(true)-$clockstart;
		}
	}
	
	function logtime($str)
	{
		if (!DEBUG_MODE)
			return;
		
		global $timelog;
		global $lastlogtime;
		
		$time = run_time(true);
		$timediff = number_format($time-$lastlogtime,3);
		$timelog .= sprintf("%20s - %s (+%s)\n", $str, number_format($time, 3), $timediff);
		$lastlogtime = $time; 
	}
	
	function dumplog()
	{
		if (!DEBUG_MODE)
			return;
		
		global $timelog;
		
		echo $timelog;
	}
	
	function output($s)
	{
		global $output_buf;
		global $ignoreline;
		
		if (!$ignoreline) {
			$output_buf .= $s;
		}
	}
	
	function output_setval($v)
	{
		global $output_val;
		
		$output_val .= $v;
	}
	
	function output_finish()
	{
		global $output_arr;
		global $output_buf;
		global $output_val;
		global $line_type;
		
		$ov = (int) trim($output_val);
		if ($line_type == "empty") $ov = -1;
		array_push($output_arr, array( 0 => $output_buf, 1 => $ov, 2 => count($output_arr) ));
		
		$output_buf = "";
		$output_val = "";
	}
	
	function cmp_rows($a, $b)
	{
		if ($a[1] == $b[1]) {
			return ($a[2] < $b[2]) ? -1 : 1;
		}
		else return ($a[1] > $b[1] ? -1 : 1);
	}
	
	function output_dump()
	{
		global $output_arr;
		global $brief_listing;
		
		if ($brief_listing) {
			usort($output_arr, "cmp_rows");
		}
		
		foreach ($output_arr as $r)
		{
			echo $r[0];
		}
		
		$output_arr = array();
	}
	
	function isCachedImg($imgsrc)
	{
		if (preg_match('&^levelshots/([a-z0-9]+\.jpg)$&',$imgsrc,$matches)) {
			if (file_exists('levelshots/'.$matches[1])) {
				return true;
			}
		}
		return false;
	}
	
	function startElement($parser, $name, $attrs)
	{
		global $intable;
		global $tablelev;
		global $cururl;
		global $ignoreline;
		global $brief_listing;
		global $inobservers;
		global $line_type;
		
		if ($tablelev == 1 && $name == "tr") {
			if (strpos(@$attrs["class"], "nebottom") !== false) $line_type = "second";
			else if (strpos(@$attrs["class"], "netop") !== false) $line_type = "first";
			else $line_type = "empty";
		}

		// we are entering a row which represents an empty server
		// so if user wants, we will set ignore flag on
		if ($name == "tr" && $brief_listing && $tablelev == 1) {
			$ignoreline = $line_type == "empty";
		}
		
		if ($ignoreline) {
			return;
		}
		
		if (@$attrs["class"] == "observers" || $inobservers) {
			$inobservers = $name;
		}
		
		if ($intable) {
			output("<".$name);
			
			foreach($attrs as $k => $v) {
				if ($k == "href" || $k == "src") {
					if ($v[0] == "/") {
						$v = substr($v, 1);
					}

					if ($name != 'img' || $k != 'src' || !isCachedImg($v)) {
						$v = $cururl.$v;
					}
				}
				output(' '.htmlspecialchars($k).'="'.htmlspecialchars($v).'"');
			}
			
			output(">");
		}
		
		if (!$intable && @$attrs["id"] == "nowplaying") {
			$intable = true;
		}
		
		if ($intable && $name == "table") {
			$tablelev++;
		}		
	}
	
	function endElement($parser, $name)
	{
		global $intable;
		global $tablelev;
		global $ignoreline;
		global $inobservers;
		global $line_type;
		
		if ($ignoreline) {
			return;
		}
		
		if ($intable && $name == "table") {
			$tablelev--;
			if (!$tablelev) {
				$intable = false;
			} 
		}
		
		if ($intable) {
			output ("</".$name.">");
		}
		
		if ($name == "tr" && $tablelev == 1 && ($line_type == "second" || ($line_type == "empty"))) {
			output_finish();
		}
		
		if ($inobservers == $name) {
			$inobservers = false;
		}
	}
	
	function cdata($parser, $d) {
		global $intable;
		global $ignoreline;
		global $inobservers;
		
		if ($intable) {
			output($d);
		}
		
		if ($inobservers == "span" && !$ignoreline) {
			output_setval($d);
		}
	}
	
	function defaultdata($parser, $data)
	{
		output($data);
		return TRUE;
	}
	
/**
  * Translate literal entities to their numeric equivalents and vice
versa.
  *
  * PHP's XML parser (in V 4.1.0) has problems with entities! The only
one's that are recognized
  * are &amp;, &lt; &gt; and &quot;. *ALL* others (like &nbsp; &copy;
a.s.o.) cause an 
  * XML_ERROR_UNDEFINED_ENTITY error. I reported this as bug at
http://bugs.php.net/bug.php?id=15092
  * The work around is to translate the entities found in the XML source
to their numeric equivalent
  * E.g. &nbsp; to &#160; / &copy; to &#169; a.s.o.
  * 
  * NOTE: Entities &amp;, &lt; &gt; and &quot; are left 'as is'
  * 
  * @author Sam Blum bs_php@users.sourceforge.net
  * @param string $xmlSource The XML string
  * @param bool   $reverse (default=FALSE) Translate numeric entities to
literal entities.
  * @return The XML string with translatet entities.
  */
	function _translateLiteral2NumericEntities($xmlSource, $reverse = FALSE)
	{
		static $literal2NumericEntity;
	
		if (empty($literal2NumericEntity)) {
			$transTbl = get_html_translation_table(HTML_ENTITIES);
			foreach ($transTbl as $char => $entity) {
				if (strpos('&"<>', $char) !== FALSE) continue;
				$literal2NumericEntity[$entity] = '&#'.ord($char).';';
			}
		}
		if ($reverse) {
			return strtr($xmlSource, array_flip($literal2NumericEntity));
		} else {
			return strtr($xmlSource, $literal2NumericEntity);
		}
	}
	
	function InsertUrl($url, $data)
	{
		global $xml_parser;
		global $intable;
		global $tablelev;
		global $cururl;
		global $errors;
		global $ignoreline;

		$intable = false;
		$tablelev = 0;
		$ignoreline = false;
		$cururl = $url;
		
		if ($data === false) {
			$errors .= "<p>Couldn't open {$url}</p>";
			return;
		}
		$data = _translateLiteral2NumericEntities($data);
		
		$xml_parser = xml_parser_create();
		if (!$xml_parser) {
			$errors .= "<p>Couldn't create XML parser</p>";
			return;
		}
		xml_parser_set_option($xml_parser, XML_OPTION_CASE_FOLDING, false);
		xml_set_element_handler($xml_parser, "startElement", "endElement");
		xml_set_character_data_handler($xml_parser, "cdata");
		xml_set_default_handler($xml_parser, "defaultdata");
		if (!xml_parse($xml_parser, $data, true)) {    
			$err = xml_get_error_code($xml_parser);
			$errors .= "<p>XML Parser returned error ".$err."</p>";
		  $errors .= "<p>".xml_error_string($err)."</p>";
		}
		xml_parser_free($xml_parser);
	}
	
	function ProcessRSS($url, $data, $observers_limit)
	{
		$xml = new SimpleXMLElement($data);
		foreach ($xml->channel->item as $stream) {
			$observers_count = ((int) ((string) $stream->observercount));
			if ($observers_count >= $observers_limit) {
				echo $stream->asXML();
			}
		}
	}
	
	if ($request_type == "htmlactive" || $request_type == "htmlfull") { 
		include ROOT."/inc/header.php";
		echo '<table id="nowplaying" cellspacing="0">';
		$commentaryBanner = new CommentaryBanner();
		if ($commentaryBanner->isEnabled()) {
			echo '<tr class=""><td colspan="3">Tune-in for live commentary: <strong><a href="'.COMMENTARY_URL.'">'.COMMENTARY_URL.'</a></strong></td></tr>';
		}
		flush();
	}
	else if ($request_type == "rss") {
		include ROOT."/inc/header_rss.php";
		flush();
	}

	$streams = array();
	$stream_data = array();
	foreach ($qtvlist as $id => $server) {
		if (!$server->isEnabled()) continue;
		$url = "http://{$server->hostname}:{$server->port}/";
		logtime($url);
		logtime("before fsockopen");
		$stream = fsockopen($server->ip, $server->port, $errno, $errstr, SOCKET_OPEN_TIMEOUT);
		if ($stream !== false) {
			if ($server->errors > 0 && mt_rand(0,100000) < ERROR_REDUCTION_COEF*100000) {
				$model->decreaseErrors($id);
			}
			if (!stream_set_timeout($stream, SOCKET_READ_TIMEOUT)) {
				trigger_error("stream_set_timeout failed for ".$url, E_USER_ERROR);
			}
			fwrite($stream, "GET {$requested_page} HTTP/1.1\nHost: {$server->hostname}:{$server->port}\n\n");
			$streams[] = array(STREAM_ID => $stream, STREAM_URL => $url);
			$stream_data[$url] = "";
		}
		else {
			logtime("fsockopen failed");

			$model->increaseErrors($id);
			$server->errors++;

			if ($server->errors >= MAX_ERRORS) {
				$model->errorServer($id);
				if (defined('ADMIN_EMAIL') && strlen('ADMIN_EMAIL')) {
					mail(ADMIN_EMAIL, "Server removed from MetaQTV", 
						"Server http://{$id}/ was removed from MetaQTV @ ".$_SERVER['SERVER_NAME']." due to high error count.\n"
						."This is automated message, do not reply.\n",
						"From: metaqtv@{$_SERVER['SERVER_NAME']}");
				}
			}
		}
		logtime("afterfsockopen");
	}
	
	logtime("before reading");
	while (count($streams)) {
		foreach ($streams as $sid => $stream) {
			$data = fread($stream[STREAM_ID], 1500);
			if ($data !== false) {
				$stream_data[$stream[STREAM_URL]] .= $data;
			}
			if (feof($stream[STREAM_ID])) {
				fclose($stream[STREAM_ID]);
				unset($streams[$sid]);
			}
		}
	}
	
	logtime("after reading");

	/// XXX refactor this for less repetition of the code
	switch ($request_type) {
		case "htmlactive": {
			foreach ($qtvlist as $id => $server) {
				if (!$server->isEnabled()) continue;
				$url = "http://{$server->hostname}:{$server->port}/";
				$name = $server->name;
				if (isset($stream_data[$url])) {
					InsertURL($url, strstr($stream_data[$url],"<?xml"));
					logtime($name);
				}
			}
			output_dump();
		}
		break;
		
		case "htmlfull": {
			foreach ($qtvlist as $id => $server) {
				if (!$server->isEnabled()) continue;
				$url = "http://{$server->hostname}:{$server->port}/";
				$name = $server->name;
				if (isset($stream_data[$url])) {
					echo "<tr class='qtvsep'><td colspan='3'><a href='".htmlspecialchars($url)."'>".htmlspecialchars($name)."</a>";
					
					if (preg_match("-<h2>(.*)</h2>-", $stream_data[$url], $matches) == 1) {
						$subheader = trim($matches[1]);
						if (strlen($subheader) > 0) {
							echo "<br />\n\t<span class='subheader'>{$matches[1]}</span>\n";
						}
					}
					
					echo "</td></tr>";
					InsertURL($url, strstr($stream_data[$url],"<?xml"));
					output_dump();
					flush();
				}
			}
		}
		break;
		
		case "rss": {
			foreach ($qtvlist as $id => $server) {
				if (!$server->isEnabled()) continue;
				$url = "http://{$server->hostname}:{$server->port}/";
				$name = $server->name;
				if (isset($stream_data[$url])) {
					ProcessRSS($url, strstr($stream_data[$url],"<?xml"), $observers_limit);
				}			
			}
		}
		break;
	}

	if ($request_type == "htmlfull" || $request_type == "htmlactive") {
		if (DEBUG_MODE && strlen($errors)) {
			echo "</table>";
			echo "<div id='errors'>".$errors."</div><table>\n";
		}
	
		echo "</table>\n";
		include ROOT."/inc/footer.php";
	}
	else if ($request_type == "rss") {
		echo "</channel>\n";
		echo "</rss>\n";
	}
	
	if (DEBUG_MODE) {
		echo "<!--\n";
		dumplog();
		echo "-->\n";
	}
