#![no_std]

use core::ffi::c_char;

// TRON Monitor API wrapper
extern "C" {
    fn tm_putstring(str: *const c_char) -> i32;
}

#[no_mangle]
pub extern "C" fn rust_hello() {
    unsafe {
        tm_putstring(b"Hello from Rust!\n\0".as_ptr() as *const c_char);
    }
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}
