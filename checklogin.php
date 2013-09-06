<?php

$host="localhost"; // Host name
$username="root"; // Mysql username
$password="cxphong"; // Mysql password
$db_name="boards"; // Database name
$tbl_name="login"; // Table name

// Connect to server and select databse.
mysql_connect("$host", "$username", "$password")or die("cannot connect");
mysql_select_db("$db_name")or die("cannot select DB");

// username and password sent from form
$myID      =$_POST['myID'];
$myusername=$_POST['myusername'];
$mypassword=$_POST['mypassword'];

// To protect MySQL injection (more detail about MySQL injection)
$myID = stripslashes($myID);
$myusername = stripslashes($myusername);
$mypassword = stripslashes($mypassword);
$myID = mysql_real_escape_string($myID);
$myusername = mysql_real_escape_string($myusername);
$mypassword = mysql_real_escape_string($mypassword);

$sql="SELECT * FROM $tbl_name WHERE username='$myusername' and password='$mypassword' and  ID='$myID'";
$result=mysql_query($sql);

// Mysql_num_row is counting table row
$count=mysql_num_rows($result);

// If result matched $myusername and $mypassword, table row must be 1 row

if($count==1){

// Register $myusername, $mypassword and redirect to file "login_success.php"
session_register("myID");
session_register("myusername");
session_register("mypassword");
header("location:blink_led.php");
}
else {
echo "Wrong ID or Username or Password";
}

?>
