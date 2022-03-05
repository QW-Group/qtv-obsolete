<?php
/**
 *
 * QuakeWorld Server Address Aliases Generator
 * 
 * @author vikpe
 *
 */    

$servers = array();

function http_get($url)
{
    $url_stuff = parse_url($url);
    $port = isset($url_stuff['port']) ? $url_stuff['port'] : 80;

    $fp = fsockopen($url_stuff['host'], $port);

	$get = ( $url_stuff['query'] ) ? '?' . $url_stuff['query'] : '';

    $query  = 'GET ' . $get . " HTTP/1.0\n";
    $query .= 'Host: ' . $url_stuff['host'];
    $query .= "\n\n";

    fwrite($fp, $query);

    while ($tmp = fread($fp, 1024)){
        $buffer .= $tmp;
    }

	$start = '<?xml version="1.0"?>';

	$buffer = substr($buffer, strpos($buffer, $start));

	return $buffer;
}

class QTVServer
{
	var $hostname	= '';
	var $port		= '';
	var $qtv		= '';

	function __construct($obj)
	{
		$this->hostname = $obj->hostname;
		$this->port	= $obj->port;

		if ( $this->hostname == '' )
		{
			list($hostname, $port) = explode(':', $obj->title);

			$this->hostname = $hostname;
			$this->port = $port;
		}

		$url = parse_url($obj->link);

		$this->qtv = str_replace('sid=', '', $url['query']) . '@' . $url['host'] . ':' . $url['port'];
	}

	function writeAlias($nr='')
	{
		$server = $this->hostname . ':' . $this->port;

		$ignore = array();

		$allpieces = explode('.', $this->hostname);
		$pieces = array();

		$ignore = array('cc', 'com', 'cz', 'dk', 'fi', 'mine', 'net', 'de', 'nl', 'nu', 'org', 'pl', 'me', 'quake', 'qw', 'ru', 'se', 'q');

		foreach ( $allpieces as $piece )
		{
			if ( !in_array($piece, $ignore) ){
				$pieces[] = $piece;
			}
		}
		$title = end($pieces);
		$title2 = $title . $nr;

		$tab2 = "\t\t";
		$tab3 = "\t\t\t";
		$nl = "\"\n";

		echo 'tempalias svr_' . $title2 . $tab3 . '"_svr_connect ' . $server . '; _qtv_set ' . $this->qtv . $nl;
		echo 'tempalias svr_' . $title2 . '_say' . $tab3 . '"say ' . $server . $nl;
		echo 'tempalias svr_' . $title2 . '_qtv' . $tab3 . '"_qtv_play ' . $this->qtv . $nl;
		echo 'tempalias svr_' . $title2 . '_qtv_say' . $tab2 . '"say ' . $title . ' #' . $nr . ' QTV: qtvplay ' . $this->qtv . $nl;
	}
}

class RSSParser
{
	public static $servers = array();
	private static $inside_item = false;

	var $id				= '';
    var $title			= '';
    var $link 			= '';
    var $description 	= '';
	var $hostname		= '';
	var $port			= '';

	function startElement( $parser, $name, $attrs='' )
	{
		global $current_tag;
		$current_tag = $name;

		if ( $current_tag == 'ITEM' )
		{
			self::$inside_item = true;
		}

	} // endfunc startElement

	function endElement( $parser, $tagName, $attrs='' )
	{
    	if ( $tagName == 'ITEM' )
		{
			if ( preg_match('/[a-z.]+:\d+/ix', $this->title) )
			{
				global $servers;

				$servers[] = new QTVServer($this);
			}

			// echo '<br>'; print_r($this); echo '<br>';

			$this->id = '';
    		$this->title = '';
    		$this->description = '';
    		$this->link = '';
			$this->hostname = '';
			$this->port = '';

    		self::$inside_item = false;
    	}

	} // endfunc endElement

	function characterData( $parser, $data )
	{
		global $current_tag;

		$data = trim(str_replace(array("\n","\t"), "", $data));

		if( self::$inside_item )
		{
			switch ( $current_tag )
			{
				case 'ID':
					$this->id .= $data;
					break;
				case 'TITLE':
					$this->title .= $data;
					break;
				case 'DESCRIPTION':
					$this->description .= $data;
					break;
				case 'LINK':
					$this->link .= $data;
					break;
				case 'HOSTNAME':
					$this->hostname .= $data;
					break;
				case 'PORT':
					$this->port .= $data;
					break;

				default:
					break;

			} // endswitch

		} // end if

	} // endfunc characterData

	function parse_results( $xml_parser, $rss_parser, $file )
	{
		xml_set_object( $xml_parser, &$rss_parser );
		xml_set_element_handler( $xml_parser, 'startElement', 'endElement' );
		xml_set_character_data_handler( $xml_parser, 'characterData' );

		$data = http_get($file);

		// parse the data
		xml_parse( $xml_parser, $data) or die( sprintf( "XML error: %s at line %d", xml_error_string(xml_get_error_code($xml_parser) ), xml_get_current_line_number( $xml_parser ) ) );
		xml_parser_free( $xml_parser );

	} // endfunc parse_results
} // endclass RSSParser

global $rss_url;

// Set feed
$rss_url = "http://qtv.quakeworld.nu/?rss";

$xml_parser = xml_parser_create();
$rss_parser = new RSSParser();

$rss_parser->parse_results( $xml_parser, &$rss_parser, $rss_url );

$lasthost = '';

$n = count($servers);
$nl = "\n";

echo '<pre>';
for ( $i=0; $i<$n; $i++ )
{
	if ( $servers[$i]->hostname == $lasthost )
	{
		$j++;
	}
	else
	{
		echo $nl;
		$j=1;
	}

	$servers[$i]->writeAlias($j);

	$lasthost = $servers[$i]->hostname;
}



echo $nl;
echo 'tempalias proxy_wargames			"_proxy_connect quake.wargamez.dk:30000"' . $nl;
echo 'tempalias proxy_wargames_say		"say quake.wargamez.dk:30000 (proxy)"' . $nl;
echo $nl;
echo 'tempalias proxy_intarweb			"_proxy_connect qw.intarweb.dk:30000"' . $nl;
echo 'tempalias proxy_intarweb_say		"say qw.intarweb.dk:30000 (proxy)"' . $nl;
echo $nl;
echo 'tempalias proxy_troopers			"_proxy_connect troopers.fi:30000"' . $nl;
echo 'tempalias proxy_troopers_say		"say troopers.fi:30000 (proxy)"' . $nl;
echo $nl;
echo 'tempalias proxy_quakeworld_fi		"_proxy_connect quakeworld.fi:30000"' . $nl;
echo 'tempalias proxy_quakeworld_fi_say		"say quakeworld.fi:30000 (proxy)"' . $nl;
echo $nl;
echo 'tempalias proxy_quakeservers		"_proxy_connect qw.fi.quakeservers.net:30000"' . $nl;
echo 'tempalias proxy_quakeservers_say		"say qw.fi.quakeservers.net:30000 (proxy)"' . $nl;
echo $nl;
echo 'tempalias proxy_quake_fi			"_proxy_connect quake.fi:30000"' . $nl;
echo 'tempalias proxy_quake_fi_say		"say quake.fi:30000 (proxy)"' . $nl;
echo $nl;
echo 'tempalias proxy_pangela			"_proxy_connect pangela.se:30000"' . $nl;
echo 'tempalias proxy_pangela_say		"say pangela.se:30000 (proxy)"' . $nl;
echo $nl;
echo 'tempalias proxy_fog			"_proxy_connect fog.mine.nu:30000"' . $nl;
echo 'tempalias proxy_fog_say			"say fog.mine.nu:30000 (proxy)"' . $nl;
echo $nl;
echo 'set prxaddress		"null"' . $nl;
echo 'set qtvaddress		"null"' . $nl;
echo 'set svraddres		"null"' . $nl;
echo $nl;
echo 'tempalias qtv			"if $qtvaddress != null then qtvplay $qtvaddress else echo No QTV-stream set for the current server"' . $nl;
echo 'tempalias qtv_say			"if $qtvaddress != null then say_game $qtvaddress else echo No QTV-stream set for the current server"' . $nl;
echo $nl;
echo 'tempalias svr_say			"if $svraddress != null then say $svraddress else echo No server set"' . $nl;
echo 'tempalias svr_reconnect		"if $svraddress != null then connect $svraddress else echo No server set"' . $nl;
echo $nl;
echo 'tempalias proxy_say		"if $prxaddress != null then say $prxaddress else echo Not using proxy"' . $nl;
echo 'tempalias proxy_saveip		"setinfo prx $lastip"' . $nl;
echo $nl;
echo 'tempalias _prx_set		"setinfo prx %1"' . $nl;
echo $nl;
echo 'tempalias _proxy_set		"set prxaddress %1"' . $nl;
echo 'tempalias _proxy_connect		"_proxy_set %1; connect %1"' . $nl;
echo $nl;
echo 'tempalias _svr_set		"set svraddress %1; _prx_set $svraddress"' . $nl;
echo 'tempalias _svr_connect		"_svr_set %1; set qtvaddress null; connect $svraddress"' . $nl;
echo $nl;
echo 'tempalias _qtv_set		"set qtvaddress %1"' . $nl;
echo 'tempalias _qtv_play		"_qtv_set %1; qtvplay $qtvaddress"' . $nl;
echo '</pre>';

?>
