<h1>MetaQTV Administration</h1>

<p><a href="../">MetaQTV</a> | <a href=".">Refresh</a> | Logged in as: <?php echo $_SERVER['PHP_AUTH_USER']; ?></p>

<h2>Live commentary banner</h2>
<p>Live commentary announcement banner is  
<?php if (isCommentaryBannerOn()): ?>
enabled
<?php else: ?>
disabled
<?php endif; ?>
</p>
<form action="" method="post">
<input type="hidden" name="act" value="toggle_banner"/>
<input type="submit" value="toggle banner"/>
</form>

<h2>Servers</h2>
<table>
<thead><tr>
	<th>id</th>
	<th>order</th><th>name</th><th>hostname</th><th colspan='2'>ip</th><th>port</th>
	<th>state</th><th>errors</th><th>toggle</th><th colspan='2'>move</th><th>remove</th>
</tr></thead>
<tbody>
<?php foreach (getServerList() as $id => $server) : ?>
<tr>
	<th><?php echo $id; ?></th>
	<td><?php echo $server->order; ?></td>
	<td><?php echo $server->name; ?></td>
	<td><a href="http://<?php echo $server->hostname; ?>:<?php echo $server->port; ?>/"><?php echo $server->hostname; ?></a></td>
	<td><form action="" method="post">
		<input type="hidden" name="act" value="updateip" />
		<input type="hidden" name="server" value="<?php echo $id; ?>" />
		<input type="submit" value="update" /></form></td>
	<td><a href="http://<?php echo $server->ip; ?>:<?php echo $server->port; ?>/"><?php echo $server->ip; ?></a></td>
	<td><?php echo $server->port; ?></td>
	<td><?php echo $server->stateName(); ?></td>
	<td><?php echo $server->errors; ?></td>
	<td><?php if ($server->isEnabled()) : ?>
		<form action="" method="post">
		<input type="hidden" name="act" value="disable" />
		<input type="hidden" name="server" value="<?php echo $id; ?>" />
		<input type="submit" value="disable" />
		</form>		
		<?php else : ?>
		<form action="" method="post">
		<input type="hidden" name="act" value="enable" />
		<input type="hidden" name="server" value="<?php echo $id; ?>" />
		<input type="submit" value="enable" />
		</form>
		<?php endif ?>
	</td>
	<td><form action="" method="post">
		<input type="hidden" name="act" value="setorder" />
		<input type="hidden" name="server" value="<?php echo $id; ?>" />
		<input type="hidden" name="order" value="<?php echo $server->order-1; ?>" />
		<input type="submit" value="up" /></form></td>
	<td><form action="" method="post">
		<input type="hidden" name="act" value="setorder" />
		<input type="hidden" name="server" value="<?php echo $id; ?>" />
		<input type="hidden" name="order" value="<?php echo $server->order+1; ?>" />
		<input type="submit" value="down" /></form></td>
	<td><form action="" method="post">
		<input type="hidden" name="act" value="delete" />
		<input type="hidden" name="server" value="<?php echo $id; ?>" />
		<input type="submit" value="delete" /></form></td>
</tr>
<?php endforeach ?>
</tbody>
</table>

<h2>Add Server</h2>
<?php
	formStart("add");
	formAddText("Hostname:", "hostname");
	formAddText("Port:", "port");
	formAddText("Name:", "name");
	formEnd();
?>
