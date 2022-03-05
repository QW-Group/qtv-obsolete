<?php

function http_get($url)
{
    $url_stuff = parse_url($url);
    $port = isset($url_stuff['port']) ? $url_stuff['port'] : 80;

    $fp = fsockopen($url_stuff['host'], $port);

    $query  = 'GET ' . $url_stuff['path'] . " HTTP/1.0\n";
    $query .= 'Host: ' . $url_stuff['host'];
    $query .= "\n\n";

    fwrite($fp, $query);

    while ($tmp = fread($fp, 1024)){
        $buffer .= $tmp;
    }

	return substr($buffer, strpos($buffer, '<?xml version="1.0"'));
}

class RSSParser
{
	var $id				= "";
    var $title			= "";
    var $link 			= "";
    var $description 	= "";
    var $inside_item 	= false;

	function startElement( $parser, $name, $attrs='' )
	{
		global $current_tag;
		$current_tag = $name;
		if ( $current_tag == "ITEM" ) $this->inside_item = true;

	} // endfunc startElement

	function endElement( $parser, $tagName, $attrs='' )
	{
		global $current_tag;

    	if ( $tagName == "ITEM" )
		{
			if ($this->id == 'separator') : 
		?>
<div class="qwnet_separator">&nbsp;</div>
		<?php else : ?>
<a href="<?php echo htmlentities($this->link); ?>" title="<?php echo $this->title; ?> - <?php echo $this->description; ?>" id="qwnet_<?php echo $this->id; ?>"<?php if ( preg_match("/".preg_quote($this->link, '/')."/i", 'http://'.$_SERVER["HTTP_HOST"].$_SERVER["REQUEST_URI"]) ) : ?> class="qwnet_current"<?php endif;?>></a>
		<?php
			endif;

			$this->id = "";
    		$this->title = "";
    		$this->description = "";
    		$this->link = "";
    		$this->inside_item = false;
    	}

	} // endfunc endElement

	function characterData( $parser, $data )
	{
		global $current_tag;

		$data = str_replace(array("\n","\t"), "", $data);

		if( $this->inside_item )
		{
			switch($current_tag)
			{
				case "ID":
					$this->id .= $data;
					break;
				case "TITLE":
					$this->title .= $data;
					break;
				case "DESCRIPTION":
					$this->description .= $data;
					break;
				case "LINK":
					$this->link .= $data;
					break;

				default:
					break;

			} // endswitch

		} // end if

	} // endfunc characterData

	function parse_results( $xml_parser, $rss_parser, $file )
	{
		xml_set_object( $xml_parser, &$rss_parser );
		xml_set_element_handler( $xml_parser, "startElement", "endElement" );
		xml_set_character_data_handler( $xml_parser, "characterData" );

		$data = http_get($file);

		// parse the data
		xml_parse( $xml_parser, $data) or die( sprintf( "XML error: %s at line %d", xml_error_string(xml_get_error_code($xml_parser) ), xml_get_current_line_number( $xml_parser ) ) );
		xml_parser_free( $xml_parser );

	} // endfunc parse_results
} // endclass RSSParser

global $rss_url;

// Set feed
$rss_url = "http://network.quakeworld.nu/bar/rss.php";

$xml_parser = xml_parser_create();
$rss_parser = new RSSParser();

?>

<div id="qwnetwork">
	<div id="qwnet_content">
		<?php $rss_parser->parse_results( $xml_parser, &$rss_parser, $rss_url ) ?>
<div id="qwnet_clearer">&nbsp;</div>
	</div>
</div>
