--TEST--
cairo_ps_surface_dsc_begin_setup() function
--SKIPIF--
<?php 

if(!extension_loaded('cairo_wrapper')) die('skip ');

if(!function_exists('cairo_ps_surface_dsc_begin_setup')) die('skip not compiled in (CAIRO_HAS_PS_SURFACE)');

 ?>
--FILE--
<?php
echo 'OK'; // no test case for this function yet
?>
--EXPECT--
OK