<?php
	define ('ROOT', '..');
	require_once ROOT."/conf/config.php";
	require_once ROOT."/inc/model.php";

	function posted($key) {
		return isset($_POST[$key]);
	}

	function addServer() {
		if (posted("hostname") && posted("port") && posted("name")) {
			$list = new ServerList;
			$list->addServer($_POST["hostname"], (int) $_POST["port"], $_POST["name"]);
		}
	}
	
	function refresh() {
		header("Location: index.php");
		exit;
	}
	
	function enableServer() {
		if (posted("server")) {
			$list = new ServerList;
			$list->enableServer($_POST["server"]);
		}
	}

	function disableServer() {
		if (posted("server")) {
			$list = new ServerList;
			$list->disableServer($_POST["server"]);
		}
	}
	
	function formStart($action) {
		echo "<form action='' method='post'>\n<input type='hidden' name='act' value='{$action}' />\n<table>";
	}
	
	function formAddText($label, $name) {
		echo "<tr><td><label for='{$name}'>{$label}</label></td><td><input type='text' name='{$name}' /></td></tr>\n";
	}
	
	function formEnd() {
		echo "<tr><td colspan='2'><input type='submit' value='Submit' /></td></tr>\n</table></form>";
	}
	
	function setOrder() {
		if (posted("server") && posted("order")) {
			$list = new ServerList;
			$list->setServerOrder($_POST["server"], (int) $_POST["order"]);
		}
	}
	
	function delete() {
		if (posted("server")) {
			$list = new ServerList;
			$list->deleteServer($_POST["server"]);
		}
	}
	
	function getServerList() {
		$list = new ServerList;
		return $list->getList();
	}
	
	function isCommentaryBannerOn() {
		$commentaryBanner = new CommentaryBanner();
		return $commentaryBanner->isEnabled();
	}
	
	function toggleCommentaryBanner() {
		$commentaryBanner = new CommentaryBanner();
		if ($commentaryBanner->isEnabled()) {
			$commentaryBanner->disable();
		}
		else {
			$commentaryBanner->enable();
		}
	}
	
	function updateIPAddress() {
		if (posted("server")) {
			$list = new ServerList;
			$list->updateServerIPAddress($_POST["server"]);
		}
	}
	
	function main() {
		if (!isset($_POST["act"])) {
			include "../inc/admin-forms.php";
		}
		else { // action
			switch ($_POST["act"]) {
			case "add":	
				addServer();
				include "../inc/admin-forms.php";
				break;
			case "logout":
				logout();
				refresh();
				break;
			case "enable":
				enableServer();
				refresh();
				break;
			case "disable":
				disableServer();
				refresh();
				break;
			case "setorder":
				setOrder();
				refresh();
				break;
			case "delete":
				delete();
				refresh();
				break;
			case "updateip":
				updateIPAddress();
				refresh();
				break;
			case "toggle_banner":
				toggleCommentaryBanner();
				refresh();
				break;
			default:
				echo "Invalid request\n";
				break;
			}
		}	
	}
	
	main();
	
?>

