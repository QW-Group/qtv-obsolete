<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Transitional//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd'>
<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en' lang='en'>
<head>
  <meta http-equiv="Content-type" content="text/html; charset=utf-8" />
  <title>QuakeTV: Now Playing</title>
  <link rel="StyleSheet" href="style.css" type="text/css" />
  <link rel="StyleSheet" href="<?php echo TOPBAR_CSS; ?>" type="text/css" />
  <link rel="alternate" type="application/rss+xml" title="RSS" href="?rss" />
</head>
<body>
<?php @include(TOPBAR_ADDR); ?>
<div id="toolbar">
<a href="?rss&amp;limit=1" title="RSS Feed"><img src="img/rss.png" /></a>
<a href="aliases.php" title="Server Aliases"><img src="img/txt.png" /></a>
<a href="help.php" title="Help"><img src="img/help.png" /></a>
</div>
<h1>QuakeTV</h1>
<p id="subtitle">Live broadcasts from all around the world<br /><span id="mainmenu">
<?php if (strpos($_SERVER['PHP_SELF'], 'help.php') !== false): ?>
	<a href=".">Back</a>
<?php else: ?>
	<?php if (!isset($_GET['full'])): ?>
		<a href=".">Refresh</a> | Active only: <a href="?full">on</a>
	<?php else: ?>
		<a href="?full">Refresh</a> | Active only: <a href=".">off</a>
	<?php endif; ?>
<?php endif; ?> 
</span></p>
