<html>
<head>
<title>Blink Leds</title>
</head>

<?PHP
	
	if (isset($_POST['Submit1']))
	{
		$host="localhost"; // Host name
		$username="root"; // Mysql username
		$password="cxphong"; // Mysql password
		$db_name="boards"; // Database name
		$tbl_name="panda_board_connecting"; // Table name
		$ID='c8:a0:30:ae:e4:50';

		// Connect to server and select databse.
		mysql_connect("$host", "$username", "$password")or die("cannot connect");
		mysql_select_db("$db_name")or die("cannot select DB");

		$answer  = $_POST['state'];  

		if ($answer == "On") 
		{          
		   	mysql_query("UPDATE $tbl_name SET Control='On'");   
		}
		else 
		{
	    	    	mysql_query("UPDATE $tbl_name SET Control='Off'"); 
		}	
	
	}
	
?>

<body>
	<form Name ="form1" Method ="POST" ACTION = "blink_led.php">
		<input type="radio" name="state" value="On">On<br>
		<input type="radio" name="state" value="Off">Off<br><br>
		<INPUT TYPE = "Submit" Name = "Submit1"  VALUE = "Submit">
	</form>
</body>

</html>
