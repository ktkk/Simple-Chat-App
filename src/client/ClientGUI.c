#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#define PORT 9000
#define MESSAGE_SIZE 2000

GtkWidget *box;
GtkWidget *stack;

pthread_t thread_id;

typedef struct {

    GtkWidget *listBox;   
    GtkWidget *scrollable;
    GtkWidget *text_entry;
    GtkEntryBuffer *buffer;
    GtkAdjustment *vadjust; 
    
    GtkWidget *loginBox;
    GtkWidget *loginText;
    GtkEntryBuffer *loginBuffer;

} WidgetData;

char   name[32]               = {0};
int    loggedIn               =  0;
int    server_socket          =  0;
int    message_position       =  0;
double current_bottom_of_page =  0;

struct sockaddr_in server_addr;


void scroll_to_bottom(GtkWidget *scrollable, GtkAdjustment *vadjust) {
    double upper       = gtk_adjustment_get_upper(vadjust);
    double page_size   = gtk_adjustment_get_page_size(vadjust);
    double current_val = gtk_adjustment_get_value(vadjust);

    double prev_bottom_of_page = current_bottom_of_page;
    current_bottom_of_page = upper - page_size;

    double messages_height_diff = current_bottom_of_page - prev_bottom_of_page;

    if ( current_val + messages_height_diff < current_bottom_of_page )
        return;

    gtk_adjustment_set_value(vadjust, current_bottom_of_page);
    gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(scrollable), vadjust);
}


void login_usr() {
    gtk_stack_set_visible_child(GTK_STACK(stack), box);
	send( server_socket , name, strlen(name), 0 );
}


void get_usr_name(GtkWidget *loginText, GtkEntryBuffer *loginBuffer) {
    const gchar *usr_name = gtk_entry_buffer_get_text( loginBuffer );
    int empty = strcmp(usr_name, "");
    
    if ( empty == 0 ) return;
    if (strlen(usr_name) > 32) {
        printf("Your name is %lu characters long. Must be 32 or less\n", strlen(usr_name));
        return;
    }

    strncpy(name, usr_name, 32);
    gtk_entry_set_text(GTK_ENTRY(loginText), "");
    login_usr();


    loggedIn = 1;
}


void display_message(const gchar *message, GtkWidget *listBox) {
    GtkEntryBuffer *input   = gtk_entry_buffer_new(NULL, MESSAGE_SIZE);
    GtkWidget      *text    = gtk_entry_new_with_buffer(input);

    gtk_entry_set_alignment(GTK_ENTRY(text), 0);
    gtk_editable_set_editable(GTK_EDITABLE(text), FALSE);
    gtk_widget_set_can_focus(GTK_WIDGET(text), FALSE);

    gtk_entry_buffer_set_text(input, message, MESSAGE_SIZE);

    gtk_list_box_insert(GTK_LIST_BOX(listBox), text, message_position++);

    gtk_widget_show_all(listBox);
}


gboolean get_enter_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    WidgetData *wd = data;

    const gchar *message = gtk_entry_buffer_get_text( wd->buffer);
    int empty = strcmp(message, "");

    if (event->keyval == GDK_KEY_Return ) {
        if ( loggedIn == 0 ) {
                get_usr_name(wd->loginText, wd->loginBuffer);
        }
        if ( empty != 0 ) {
	        display_message(message, wd->listBox);
	        send( server_socket , message, strlen(message) , 0 );
	        gtk_entry_set_text( GTK_ENTRY(wd->text_entry), "" );
	        scroll_to_bottom(wd->scrollable, wd->vadjust);
        }
        return TRUE;
    }
    return FALSE;
}


int init_connection() {

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0) {
		printf("\n Socket creation error \n");
		return -1;
	}

	server_addr.sin_family      = AF_INET;
	server_addr.sin_port        = htons(PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;

    
    int connection = connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if ( connection < 0) {
		printf("\nConnection Failed \n");
		return -1;
	}

}


void *recv_message_thread(void *data) {
    WidgetData *wd = data;

    char *s_buffer[MESSAGE_SIZE] = {0};
    while(1) {
        int r = recv( server_socket, s_buffer, sizeof(s_buffer), 0 );
        if ( r > 0 ) {
            display_message( (gchar*)s_buffer, wd->listBox );
            scroll_to_bottom( wd->scrollable, wd->vadjust );
            memset( s_buffer, 0, sizeof(s_buffer) );
        }
    }
}

void quit_window(GtkWidget *window, gpointer *data) {
    char leave_message[] = "Client Exiting. CLOSE!";
    send(server_socket, leave_message, sizeof(leave_message), 0);

    pthread_cancel(thread_id);
    gtk_main_quit();
}


int main(int argc, char **argv) {

	GtkWidget *window;

    GtkWidget *loginBox;
    GtkWidget *loginLabel;

    WidgetData wd;

	gtk_init(&argc, &argv);

    stack          = gtk_stack_new();
    wd.loginBuffer = gtk_entry_buffer_new(NULL, 32);
    loginLabel     = gtk_label_new("Enter your name(max 32)");
    wd.loginText   = gtk_entry_new_with_buffer(wd.loginBuffer);
    
    box            = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    loginBox       = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	window         = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    wd.listBox     = gtk_list_box_new();
    wd.vadjust     = gtk_adjustment_new(0, 0, 0, 0, 0, 0);
    wd.scrollable  = gtk_scrolled_window_new(NULL, wd.vadjust);
    

    wd.buffer     = gtk_entry_buffer_new(NULL, MESSAGE_SIZE);
    wd.text_entry = gtk_entry_new_with_buffer(wd.buffer);


    gtk_widget_set_halign(wd.text_entry, GTK_ALIGN_FILL);
    gtk_widget_set_valign(wd.text_entry, GTK_ALIGN_END);
    gtk_widget_set_size_request(wd.text_entry, 50,25);
    

    GtkCssProvider *p = gtk_css_provider_new();
    gtk_css_provider_load_from_path(p, "theme.css", NULL);
    gtk_style_context_add_provider_for_screen( gdk_screen_get_default(),
                                               GTK_STYLE_PROVIDER(p),
                                               GTK_STYLE_PROVIDER_PRIORITY_USER);


    gtk_widget_set_name(GTK_WIDGET(wd.text_entry), "text_entry");
    gtk_widget_set_name(GTK_WIDGET(loginBox)     , "loginBox"  );


    gtk_box_pack_start( GTK_BOX(box), wd.scrollable, 1, 1, 0);
    gtk_box_pack_start( GTK_BOX(box), wd.text_entry, 0, 1, 0);

    gtk_box_pack_start( GTK_BOX(loginBox), loginLabel  , 0, 0, 0 );
    gtk_box_pack_start( GTK_BOX(loginBox), wd.loginText, 0, 0, 0 );

    gtk_stack_add_named(GTK_STACK(stack), loginBox, "loginBox"); 
    gtk_stack_add_named(GTK_STACK(stack), box, "box");


    gtk_container_add( GTK_CONTAINER(wd.scrollable), wd.listBox);
    gtk_container_add(GTK_CONTAINER(window), stack);


    gtk_widget_add_events(window, GDK_KEY_PRESS);

	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(quit_window), NULL);
    g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(get_enter_key_press), &wd);

    gtk_window_set_default_size(GTK_WINDOW(window), 500, 500);

    gtk_widget_show_all(window);


    if ( init_connection() == -1 ) return -1;

    gtk_stack_set_visible_child(GTK_STACK(stack), loginBox);

    pthread_create(&thread_id, NULL, recv_message_thread,(void*)&wd);

	gtk_main();

    pthread_join(thread_id, NULL);

    return 0;
}
