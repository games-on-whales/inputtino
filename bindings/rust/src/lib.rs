#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

#[cfg(test)]
mod tests{

    use std::ffi::{CStr, CString};
    use super::*;

    #[test]
    fn test_inputtino_mouse(){
        let device_name = CString::new("Rusty Mouse").unwrap();
        let device_phys = CString::new("Rusty Mouse Phys").unwrap();
        let device_uniq = CString::new("Rusty Mouse Uniq").unwrap();
        let def = InputtinoDeviceDefinition {
            name: device_name.as_ptr(),
            vendor_id: 0,
            product_id: 0,
            version: 0,
            device_phys: device_phys.as_ptr(),
            device_uniq: device_uniq.as_ptr(),
        };
        let _error_handler_fn = | error_message: *const ::core::ffi::c_char, user_data: *mut ::core::ffi::c_void | {
            unsafe{ println!("Error: {:?}", CStr::from_ptr(error_message).to_str().unwrap()); }
        };
        let error_handler = InputtinoErrorHandler {
            eh: None, // TODO: InputtinoErrorHandlerFn::new(error_handler_fn) ???
            user_data: std::ptr::null_mut(),
        };

        unsafe{
            let mouse = inputtino_mouse_create(&def, &error_handler);
            assert!(!mouse.is_null());

            let mut nodes_count: core::ffi::c_int = 0;
            let nodes = inputtino_mouse_get_nodes(mouse, & mut nodes_count);
            assert!(nodes_count == 2);
            assert!(!nodes.is_null());
            // Check that the nodes start with /dev/input/event
            assert!(CString::from_raw(*nodes.offset(0)).to_str().unwrap().starts_with("/dev/input/event"));
            assert!(CString::from_raw(*nodes.offset(1)).to_str().unwrap().starts_with("/dev/input/event"));

            inputtino_mouse_destroy(mouse);
        }
    }

}
