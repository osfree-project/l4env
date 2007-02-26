
/*** INIT CONNECTION TO OVERLAY-WM SERVER ***
 *
 * \param ovl_name   name of the overlay server to connect to
 * \return           0 on success
 */
extern int ovl_input_init(char *ovl_name);


/*** REGISTER CALLBACK FUNCTION FOR BUTTON INPUT EVENTS ***
 *
 * \param cb   function that should be called for each incoming event
 * \return     0 on success
 */
extern int ovl_input_button(void (*cb)(int type, int code));


/*** REGISTER CALLBACK FUNCTION FOR MOTION INPUT EVENTS ***
 *
 * \param cb   function that should be called for each motion event
 * \return     0 on success
 */
extern int ovl_input_motion(void (*cb)(int mx, int my));

