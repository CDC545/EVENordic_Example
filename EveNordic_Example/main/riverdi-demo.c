#include <zephyr/kernel.h>
#include <stdio.h>
#include "platform.h"
#include "App_Common.h"
#include "CoPro_Cmds.h"

static Gpu_Hal_Context_t host, *phost;

static void button_task(void *p1, void *p2, void *p3)
{
    uint32_t last_toggle_time = 0;
    const uint32_t TOGGLE_COOLDOWN_MS = 300;  // Minimum time between toggles
    
    uint16_t x = DispWidth/2;
    uint16_t y = 50;
    uint8_t tag = 0;
    int16_t last_y = -1;
    int16_t start_scroll = 0;
    bool touching = false;
    uint8_t brightness = 128;  // Initial brightness value
    
    // Track last touch position
    uint16_t last_touch_x = 0;
    uint16_t last_touch_y = 0;
    
    // List parameters
    int visible_items = 6;
    int item_height = 40;
    int list_width = (DispWidth / 2) - 80;  // Half display width minus margins
    int list_x = 40;  // Keep left margin
    
    // Keep track of selected device
    int selected_device = -1;  // -1 means no selection
    
    // Slider parameters
    int slider_y = DispHeight - 50;
    int slider_width = 400;
    int slider_x = (DispWidth - slider_width) / 2;

    // Sample Bluetooth devices
    const char *devices[] = {
        "iPhone 12",
        "Samsung Galaxy S21",
        "Pixel 6",
        "AirPods Pro",
        "Sony WH-1000XM4",
        "Car Audio System",
        "Smart Watch",
        "Bluetooth Speaker",
        "Gaming Controller",
        "Wireless Keyboard"
    };
    const int num_devices = sizeof(devices) / sizeof(devices[0]);
    int scroll_offset = 0;

    // Dropdown parameters
    int dropdown_x = (DispWidth/2) + 40;  // Start from middle + margin
    int dropdown_width = (DispWidth/2) - 80;  // Same width as list
    int dropdown_y = y;
    int dropdown_height = 40;
    bool dropdown_open = false;
    int selected_dropdown_item = -1;
    
    // Use same count as devices
    const int num_dropdown_items = num_devices;

    while(1)
    {
        // Read touch data and tag
        uint32_t xy = Gpu_Hal_Rd32(phost, REG_TOUCH_SCREEN_XY);
        uint16_t rz = Gpu_Hal_Rd16(phost, REG_TOUCH_RZ);
        tag = Gpu_Hal_Rd8(phost, REG_TOUCH_TAG);  // Add this line to read the tag
        
        // Extract touch coordinates
        uint16_t touch_x = (xy >> 16) & 0xffff;
        uint16_t touch_y = xy & 0xffff;
        bool touch_detected = (rz < 1200);

        // Scale touch coordinates
        int scaled_touch_x = touch_x;  // X is already in screen coordinates
        int scaled_touch_y = (touch_y * DispHeight) / 600;  // Scale Y to match screen

        // Handle brightness slider
        if (touch_detected) {
            printk("Raw Touch X: %d, Y: %d, RZ: %d, Tag: %d\n", touch_x, touch_y, rz, tag);
            printk("Scaled Touch X: %d, Y: %d\n", scaled_touch_x, scaled_touch_y);
            
            // Convert touch coordinates to screen space
            // X is already in screen coordinates (0-1024)
            // Y needs to be scaled from 1024 to screen height
            touch_x = touch_x;  // Keep X as is
            touch_y = (touch_y * DispHeight) / 600;  // Scale Y directly
            
            printk("Screen Touch X: %d, Y: %d (Slider X: %d-%d, Y: %d±40)\n", 
                   touch_x, touch_y, 
                   slider_x, slider_x + slider_width,
                   slider_y);

            // Check if touch is in slider area
            if (touch_y >= (slider_y - 40) && touch_y <= (slider_y + 40)) {
                if (touch_x >= slider_x && touch_x <= (slider_x + slider_width)) {
                    // Calculate new brightness based on relative position in slider
                    int relative_x = touch_x - slider_x;
                    brightness = (relative_x * 255) / slider_width;
                    if (brightness > 255) brightness = 255;
                    if (brightness < 0) brightness = 0;
                    
                    // Update display brightness
                    Gpu_Hal_Wr8(phost, REG_PWM_DUTY, brightness);
                    printk("Setting brightness to: %d\n", brightness);
                }
            }
            
            // Handle list scrolling and selection
            if (touch_detected) {
                bool handled_by_list = false;  // Track if touch was handled by list

                // Only handle list touches if dropdown is not open and touch is on left side
                if (!dropdown_open && 
                    touch_x < (DispWidth/2) &&  // Only handle touches on left side
                    touch_y < slider_y - 50) {
                    // Handle scrolling
                    if (!touching) {
                        touching = true;
                        last_y = touch_y;
                        start_scroll = scroll_offset;
                    } else {
                        int delta = last_y - touch_y;
                        scroll_offset = start_scroll + (delta / item_height);
                        if (scroll_offset < 0) scroll_offset = 0;
                        int max_scroll = num_devices - visible_items;
                        if (scroll_offset > max_scroll) scroll_offset = max_scroll;
                    }

                    // Handle list item selection
                    for(int i = scroll_offset; i < MIN(num_devices, scroll_offset + visible_items); i++) {
                        int item_y = y + (i - scroll_offset) * item_height;
                        if (touch_x >= list_x && touch_x <= (list_x + list_width) &&
                            touch_y >= item_y && touch_y <= (item_y + item_height - 5)) {
                            selected_device = i;
                            printk("Selected device: %s\n", devices[selected_device]);
                            handled_by_list = true;
                            break;
                        }
                    }
                }

                // Only handle dropdown if touch wasn't handled by list
                if (!handled_by_list) {
                    // Handle dropdown button click
                    if (touch_detected && !touching) {
                        if (touch_x >= dropdown_x && 
                            touch_x <= (dropdown_x + dropdown_width) &&
                            touch_y >= dropdown_y && 
                            touch_y <= (dropdown_y + dropdown_height)) {
                            
                            uint32_t current_time = k_uptime_get_32();
                            
                            // Only toggle if enough time has passed since last toggle
                            if (current_time - last_toggle_time > TOGGLE_COOLDOWN_MS) {
                                dropdown_open = !dropdown_open;
                                last_toggle_time = current_time;
                                printk("Dropdown %s\n", dropdown_open ? "opened" : "closed");
                            }
                        }
                    }
                }
            } else {
                touching = false;
            }
        } else {
            touching = false;
        }

        /* Start frame */
        Gpu_CoCmd_Dlstart(phost);
        App_WrCoCmd_Buffer(phost, CLEAR(1,1,1));
        App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(255, 255, 255));
        App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
        
        /* Draw header */
        Gpu_CoCmd_FgColor(phost, 0x0060A9);
        Gpu_CoCmd_GradColor(phost, 0x0070C9);
        Gpu_CoCmd_Text(phost, x, 20, 31, OPT_CENTER, "Available Bluetooth Devices");

        /* Draw device list section */
        // Draw "Devices Found" header
        Gpu_CoCmd_Text(phost, list_x, y - 40, 30, 0, "Devices Found");
        
        // Draw list items
        for(int i = scroll_offset; i < MIN(num_devices, scroll_offset + visible_items); i++)
        {
            int item_y = y + (i - scroll_offset) * item_height;
            
            // Draw button with highlight if it's the selected device
            Gpu_CoCmd_FgColor(phost, (selected_device == i) ? 0x0070C9 : 0x0060A9);
            Gpu_CoCmd_Button(phost, 
                list_x, item_y, 
                list_width, item_height - 5,
                28,
                OPT_FLAT,
                devices[i]);
            
            // Check if this item was touched
            if (touch_detected && 
                touch_x >= list_x && touch_x <= (list_x + list_width) &&
                touch_y >= item_y && touch_y <= (item_y + item_height - 5)) {
                selected_device = i;
                printk("Selected device: %s\n", devices[selected_device]);
            }
        }

        // Draw selected device section
        int selection_box_y = y + (visible_items * item_height) + 30;
        int box_padding = 10;
        int box_height = 40;
        
        // Draw "Selected Device:" label
        Gpu_CoCmd_Text(phost, list_x, selection_box_y, 28, 0, "Selected Device:");
        
        // Draw text box in LVGL style
        // Draw box background
        App_WrCoCmd_Buffer(phost, COLOR_RGB(240, 240, 240));  // Light gray background
        App_WrCoCmd_Buffer(phost, BEGIN(RECTS));
        App_WrCoCmd_Buffer(phost, VERTEX2F((list_x) * 16, (selection_box_y + 30) * 16));
        App_WrCoCmd_Buffer(phost, VERTEX2F((list_x + list_width) * 16, 
                                         (selection_box_y + 30 + box_height) * 16));
        App_WrCoCmd_Buffer(phost, END());
        
        // Draw box border
        App_WrCoCmd_Buffer(phost, COLOR_RGB(180, 180, 180));  // Border color
        App_WrCoCmd_Buffer(phost, LINE_WIDTH(16));  // 1 pixel wide border
        App_WrCoCmd_Buffer(phost, BEGIN(LINE_STRIP));
        App_WrCoCmd_Buffer(phost, VERTEX2F((list_x) * 16, (selection_box_y + 30) * 16));
        App_WrCoCmd_Buffer(phost, VERTEX2F((list_x + list_width) * 16, (selection_box_y + 30) * 16));
        App_WrCoCmd_Buffer(phost, VERTEX2F((list_x + list_width) * 16, (selection_box_y + 30 + box_height) * 16));
        App_WrCoCmd_Buffer(phost, VERTEX2F((list_x) * 16, (selection_box_y + 30 + box_height) * 16));
        App_WrCoCmd_Buffer(phost, VERTEX2F((list_x) * 16, (selection_box_y + 30) * 16));
        App_WrCoCmd_Buffer(phost, END());
        
        // Draw text content
        if (selected_device >= 0) {
            Gpu_CoCmd_Text(phost, list_x + box_padding, selection_box_y + 30 + (box_height/2), 
                          29, OPT_CENTERY, devices[selected_device]);
        } else {
            App_WrCoCmd_Buffer(phost, COLOR_RGB(128, 128, 128));  // Gray text for placeholder
            Gpu_CoCmd_Text(phost, list_x + box_padding, selection_box_y + 30 + (box_height/2), 
                          29, OPT_CENTERY, "No device selected");
        }

        /* Draw scroll bar if needed */
        if (num_devices > visible_items) {
            int scroll_bar_x = list_x + list_width + 10;  // Closer to list
            int scroll_bar_width = 8;
            int scroll_bar_height = visible_items * item_height;
            int scroll_handle_height = (visible_items * scroll_bar_height) / num_devices;
            int scroll_handle_y = y + (scroll_offset * scroll_bar_height) / num_devices;

            // Draw scroll bar background
            App_WrCoCmd_Buffer(phost, COLOR_RGB(200, 200, 200));
            App_WrCoCmd_Buffer(phost, BEGIN(RECTS));
            App_WrCoCmd_Buffer(phost, VERTEX2F(scroll_bar_x * 16, y * 16));
            App_WrCoCmd_Buffer(phost, VERTEX2F((scroll_bar_x + scroll_bar_width) * 16, 
                                             (y + scroll_bar_height) * 16));
            App_WrCoCmd_Buffer(phost, END());

            // Draw scroll handle
            App_WrCoCmd_Buffer(phost, COLOR_RGB(0, 96, 169));
            App_WrCoCmd_Buffer(phost, BEGIN(RECTS));
            App_WrCoCmd_Buffer(phost, VERTEX2F(scroll_bar_x * 16, scroll_handle_y * 16));
            App_WrCoCmd_Buffer(phost, VERTEX2F((scroll_bar_x + scroll_bar_width) * 16, 
                                             (scroll_handle_y + scroll_handle_height) * 16));
            App_WrCoCmd_Buffer(phost, END());
        }


        /* Draw brightness slider */
        // Slider label and value text Y position should match slider center
        int text_y = slider_y - 8;  // Align with slider top position
        
        // Slider label
        Gpu_CoCmd_Text(phost, slider_x - 80, text_y, 28, OPT_RIGHTX | OPT_CENTERY, "Brightness");
        
        // Draw slider using built-in widget
        Gpu_CoCmd_BgColor(phost, 0xE0E0E0);  // Gray background
        Gpu_CoCmd_FgColor(phost, 0x0060A9);  // Blue progress
        Gpu_CoCmd_Slider(phost,
            slider_x, slider_y - 8,    // x, y
            slider_width, 16,          // width, height
            0,                         // options
            brightness,                // value
            255);                      // range

        // Current value
        char bright_str[8];
        snprintf(bright_str, sizeof(bright_str), "%d%%", (brightness * 100) / 255);
        Gpu_CoCmd_Text(phost, slider_x + slider_width + 20, text_y, 28, OPT_CENTERY, bright_str);

        /* Draw dropdown menu */
        // Draw dropdown header
        Gpu_CoCmd_Text(phost, dropdown_x, y - 40, 30, 0, "Available Devices");
        
        // Draw main dropdown button
        App_WrCoCmd_Buffer(phost, COLOR_RGB(240, 240, 240));  // Background
        App_WrCoCmd_Buffer(phost, BEGIN(RECTS));
        App_WrCoCmd_Buffer(phost, VERTEX2F(dropdown_x * 16, dropdown_y * 16));
        App_WrCoCmd_Buffer(phost, VERTEX2F((dropdown_x + dropdown_width) * 16, 
                                         (dropdown_y + dropdown_height) * 16));
        App_WrCoCmd_Buffer(phost, END());
        
        // Draw dropdown border
        App_WrCoCmd_Buffer(phost, COLOR_RGB(180, 180, 180));
        App_WrCoCmd_Buffer(phost, LINE_WIDTH(16));
        App_WrCoCmd_Buffer(phost, BEGIN(LINE_STRIP));
        App_WrCoCmd_Buffer(phost, VERTEX2F(dropdown_x * 16, dropdown_y * 16));
        App_WrCoCmd_Buffer(phost, VERTEX2F((dropdown_x + dropdown_width) * 16, dropdown_y * 16));
        App_WrCoCmd_Buffer(phost, VERTEX2F((dropdown_x + dropdown_width) * 16, (dropdown_y + dropdown_height) * 16));
        App_WrCoCmd_Buffer(phost, VERTEX2F(dropdown_x * 16, (dropdown_y + dropdown_height) * 16));
        App_WrCoCmd_Buffer(phost, VERTEX2F(dropdown_x * 16, dropdown_y * 16));
        App_WrCoCmd_Buffer(phost, END());

        // Draw selected text or placeholder in dropdown button
        const char* display_text = (selected_dropdown_item >= 0) ? 
                                 devices[selected_dropdown_item] : 
                                 "Select Device";
        Gpu_CoCmd_Text(phost, dropdown_x + 10, dropdown_y + (dropdown_height/2), 
                      28, OPT_CENTERY, display_text);

        // Draw dropdown arrow
        App_WrCoCmd_Buffer(phost, COLOR_RGB(0, 96, 169));
        Gpu_CoCmd_Text(phost, dropdown_x + dropdown_width - 20, 
                      dropdown_y + (dropdown_height/2), 
                      28, OPT_CENTERY, dropdown_open ? "^" : "v");

        // Draw dropdown list if open
        if (dropdown_open) {
            // Draw dropdown list background (white)
            App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 255, 255));
            App_WrCoCmd_Buffer(phost, BEGIN(RECTS));
            App_WrCoCmd_Buffer(phost, VERTEX2F(dropdown_x * 16, 
                                             (dropdown_y + dropdown_height) * 16));
            App_WrCoCmd_Buffer(phost, VERTEX2F((dropdown_x + dropdown_width) * 16, 
                                             (dropdown_y + dropdown_height * (1 + num_dropdown_items)) * 16));
            App_WrCoCmd_Buffer(phost, END());

            // Draw each item
            for (int i = 0; i < num_dropdown_items; i++) {
                int item_y = dropdown_y + dropdown_height * (i + 1);
                
                // Draw item background (light gray)
                App_WrCoCmd_Buffer(phost, COLOR_RGB(245, 245, 245));
                App_WrCoCmd_Buffer(phost, BEGIN(RECTS));
                App_WrCoCmd_Buffer(phost, VERTEX2F(dropdown_x * 16, item_y * 16));
                App_WrCoCmd_Buffer(phost, VERTEX2F((dropdown_x + dropdown_width) * 16, 
                                                 (item_y + dropdown_height) * 16));
                App_WrCoCmd_Buffer(phost, END());

                // Draw separator line
                App_WrCoCmd_Buffer(phost, COLOR_RGB(220, 220, 220));
                App_WrCoCmd_Buffer(phost, LINE_WIDTH(16));
                App_WrCoCmd_Buffer(phost, BEGIN(LINES));
                App_WrCoCmd_Buffer(phost, VERTEX2F(dropdown_x * 16, (item_y + dropdown_height) * 16));
                App_WrCoCmd_Buffer(phost, VERTEX2F((dropdown_x + dropdown_width) * 16, 
                                                 (item_y + dropdown_height) * 16));
                App_WrCoCmd_Buffer(phost, END());

                // Check for hover
                if (touch_detected && 
                    touch_x >= dropdown_x && 
                    touch_x <= (dropdown_x + dropdown_width) &&
                    touch_y >= item_y && 
                    touch_y <= (item_y + dropdown_height)) {
                    // Draw hover highlight
                    App_WrCoCmd_Buffer(phost, COLOR_RGB(0, 96, 169));
                    App_WrCoCmd_Buffer(phost, BEGIN(RECTS));
                    App_WrCoCmd_Buffer(phost, VERTEX2F(dropdown_x * 16, item_y * 16));
                    App_WrCoCmd_Buffer(phost, VERTEX2F((dropdown_x + dropdown_width) * 16, 
                                                     (item_y + dropdown_height) * 16));
                    App_WrCoCmd_Buffer(phost, END());
                    App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 255, 255));  // White text for highlighted item
                } else {
                    App_WrCoCmd_Buffer(phost, COLOR_RGB(0, 0, 0));  // Black text for normal items
                }

                // Draw item text
                Gpu_CoCmd_Text(phost, dropdown_x + 10, item_y + (dropdown_height/2), 
                              28, OPT_CENTERY, devices[i]);

                // Handle item selection in dropdown list
                if (touch_detected && 
                    touch_x >= dropdown_x && 
                    touch_x <= (dropdown_x + dropdown_width) &&
                    touch_y >= item_y && 
                    touch_y <= (item_y + dropdown_height)) {
                    // Update selection and close dropdown immediately
                    selected_dropdown_item = i;
                    dropdown_open = false;
                    printk("Selected dropdown item: %s\n", devices[i]);
                }
            }

            // Draw dropdown border
            App_WrCoCmd_Buffer(phost, COLOR_RGB(180, 180, 180));
            App_WrCoCmd_Buffer(phost, LINE_WIDTH(16));
            App_WrCoCmd_Buffer(phost, BEGIN(LINE_STRIP));
            App_WrCoCmd_Buffer(phost, VERTEX2F(dropdown_x * 16, 
                                             (dropdown_y + dropdown_height) * 16));
            App_WrCoCmd_Buffer(phost, VERTEX2F((dropdown_x + dropdown_width) * 16, 
                                             (dropdown_y + dropdown_height) * 16));
            App_WrCoCmd_Buffer(phost, VERTEX2F((dropdown_x + dropdown_width) * 16, 
                                             (dropdown_y + dropdown_height * (1 + num_dropdown_items)) * 16));
            App_WrCoCmd_Buffer(phost, VERTEX2F(dropdown_x * 16, 
                                             (dropdown_y + dropdown_height * (1 + num_dropdown_items)) * 16));
            App_WrCoCmd_Buffer(phost, VERTEX2F(dropdown_x * 16, 
                                             (dropdown_y + dropdown_height) * 16));
            App_WrCoCmd_Buffer(phost, END());
        }

        /* Display frame */
        App_WrCoCmd_Buffer(phost, DISPLAY());
        App_Flush_Co_Buffer(phost);
        Gpu_Hal_DLSwap(phost, DLSWAP_FRAME);

        k_msleep(10);
    }
}

int main(void)
{
    printk("Starting Riverdi EVE Demo\n");
    
    phost = &host;
    
    /* Init HW Hal */
    App_Common_Init(phost);
    
    /* Wait for initialization */
    k_msleep(100);
    printk("logo\n");
    /* Show logo */
    App_Show_Logo(phost);
    
    /* Set initial backlight */
    Gpu_Hal_Wr8(phost, REG_PWM_DUTY, 80);
    
    /* Configure touch */
    Gpu_Hal_Wr8(phost, REG_TOUCH_MODE, 0x03);        // Extended mode
    Gpu_Hal_Wr8(phost, REG_TOUCH_ADC_MODE, 0x01);    // Single touch
    Gpu_Hal_Wr8(phost, REG_TOUCH_OVERSAMPLE, 0x0F);  // Maximum oversampling
    Gpu_Hal_Wr16(phost, REG_TOUCH_RZTHRESH, 1200);   // Resistance threshold
    
    /* Create button demo thread */
    struct k_thread button_thread;
    static K_THREAD_STACK_DEFINE(button_stack, 4096);
    
    k_thread_create(&button_thread, button_stack,
                    K_THREAD_STACK_SIZEOF(button_stack),
                    button_task,
                    NULL, NULL, NULL,
                    K_PRIO_PREEMPT(7), 0, K_NO_WAIT);

    while(1) {
        k_msleep(1000);
    }

    return 0;
}
