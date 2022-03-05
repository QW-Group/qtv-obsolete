<?php
define ('ROOT', '.');
require ROOT."/conf/config.php";
include ROOT."/inc/header.php";

?>

<div id="helptext">
<p>This site offers you live streams of current QuakeWorld games.</p>

<ul>
<li>To watch a game click on the <strong>Watch Now!</strong> button.<br />
A download of a file will start, after that you will be asked whether you want to <strong>Open</strong> or <strong>Save</strong> the file.</li>

<li>Choose <strong>Open</strong> and locate your QuakeWorld client executable - for example <em>ezquake-gl.exe</em> in your file system.<br />
Typical location is <var>C:\Program Files\nQuake\ezquake-gl.exe</var>.</li>
</ul>

<p>To make the process easier in the future, associate *.qtv files with your QuakeWorld client's executable.</p>

<p>Note: To watch, you need to have QuakeWorld installed on your computer. To do so, visit <a href="http://nquake.com/">nQuake.com</a>.</p>

</div>

<?php include ROOT."/inc/footer.php"; ?>
