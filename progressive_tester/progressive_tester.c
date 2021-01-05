/* progressive_tester.c - Tester/Viewer of progressive images
**
** Part of MorphOS Reggae framework
** (c) 2011-2016 Michal Zukowski
*/

#define VERSION_TAG "$VER: progressive_tester 1.3 (12.03.2016)"


/// INCLUDES

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/asl.h>
#include <proto/icon.h>
#include <proto/timer.h>
#include <clib/alib_protos.h>
#include <clib/debug_protos.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>
#include <libraries/charsets.h>
#include <mui/Rawimage_mcc.h>
#define USE_INLINE_STDARG
#include <proto/multimedia.h>
#include <classes/multimedia/video.h>
#include <classes/multimedia/metadata.h>

#include "api_MUI.h"

///
/// DEFINES

#define MUIM_TesterLoadImage	    OM_Dummy+1000
#define MUIM_TesterHandleAppMessage OM_Dummy+1001
#define MUIM_TesterSaveImage        OM_Dummy+1002
#define MUIM_TesterProcessImage     OM_Dummy+1003

#define MODE_Full_Image    0
#define MODE_Line_Per_Line 1
#define MODE_Combined      2
#define MODE_Half_Image    3 

#define D(x) x

///
/// structures

struct MUI_CustomClass *ProgressiveTesterClass = NULL;

DISPATCHER_DEF(ProgressiveTester_Class_)


struct RawImageHeader
{
	ULONG Width;
    ULONG Height;
    ULONG Format;
    ULONG Size;
};


char *ppp[3] = {"Filters", "Savers", NULL};

struct ImageData
{
	char author[512];
	char title[512];
	char copyright[512];
	char comment[512];
	char cameravendor[512];
	char cameramodel[512];
	double gpslat;
	double gpsalt;
	double gpslong;
	int dpi_x;
	int dpi_y;
	int aspect_x;
	int aspect_y;
	BOOL progressive;
	int palette_len;
	ULONG *palette;
	char *img;
	char *icc;
};

typedef struct 
{
	Object *window;
	Object *maingroup;
	Object *picture_reggae;
	Object *image;
	Object *uri;
	Object *text_log;
	Object *area;
	Object *info_btn;
	Object *info_window;
	Object *info_group;
	Object *infomain;
	Object *mode_radio;
	Object *saver_gui;
	Object *filter_gui;
	Object *filter_btn;
	Object *save_btn;
	Object *save_str;

	struct RawImageHeader image_header;
	struct ImageData image_data;
	char error_line[512];

} ProgressiveTester_Data;


struct MUIP_TesterLoad             {ULONG methodid; STRPTR filename;};
struct MUIP_TesterHandleAppMessage {ULONG MethodID; APTR appmessage;};

static char *radio_modes[] = {"Full image", "Line per line", "Combined lines", "Half image", NULL};
char color[256][256];
///

/// CreateProgressiveTesterClass()

struct MUI_CustomClass *CreateProgressiveTesterClass(void)
{
	struct MUI_CustomClass *cl;
	cl = MUI_CreateCustomClass(NULL, MUIC_Application, NULL, sizeof(ProgressiveTester_Data), (APTR)DISPATCHER_REF(ProgressiveTester_Class_));
	return cl;
}

///
/// GLOBAL VARS
STRPTR filename = NULL;
///


/// ProgressiveTester_New()

IPTR ProgressiveTester_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
	ProgressiveTester_Data data_tmp;
	memset(&data_tmp, 0, sizeof(data_tmp)); 
	
	if(obj = DoSuperNew(cl, obj,
	                MUIA_Application_Title,      "Progressive Tester",
	                MUIA_Application_Copyright,  "(c) 2011-2017 Michal Zukowski",
	                MUIA_Application_Author,     "Michal Zukowski",
	                MUIA_Application_Version,    VERSION_TAG,
	              
					SubWindow, data_tmp.window  = WindowObject,
						MUIA_Window_Title, "Progressive Tester",
						MUIA_Window_ID, MAKE_ID('M','A','I','N'),    
						MUIA_Window_AppWindow, TRUE,     
						WindowContents, VGroup,
							Child, VGroup,
								Child, ColGroup(2),
									Child, Label("Path or URL:"),
									Child, PopaslObject,
										MUIA_Popasl_Type, ASL_FileRequest,
										ASLFR_TitleText, "Select image...",
										MUIA_Popstring_Button, PopButton( MUII_PopFile ),
										MUIA_Popstring_String, data_tmp.uri =  StringObject,
											StringFrame,
											MUIA_String_MaxLen, 256,
											MUIA_String_Contents, (filename)?filename:"",
											MUIA_CycleChain, 1,
											MUIA_String_AdvanceOnCR, TRUE,
										End,
									End,
									Child, Label("Mode:"),
									Child, HGroup, 
										Child, data_tmp.mode_radio = RadioObject,
											MUIA_Radio_Entries, radio_modes,
											MUIA_Group_Horiz, TRUE,
										End,
										Child, HVSpace,
									End, 
								End,
								Child, RectangleObject, 
									MUIA_Rectangle_HBar, TRUE, 
									MUIA_VertWeight,0, 
								End,
								Child, data_tmp.maingroup = HGroup,
									Child, data_tmp.area= HVSpace,
									Child, HVSpace,
								End,
							End,
						
							Child, HGroup,
								Child, data_tmp.text_log = TextObject, MUIA_Text_Contents, "Ready...", 
									MUIA_Frame, MUIV_Frame_Text, 
									MUIA_Background,  MUII_TextBack,
									MUIA_Text_Marking, TRUE,

								End,
								Child,  data_tmp.info_btn = TextObject,
									MUIA_Font, MUIV_Font_Button,
									MUIA_Text_Contents, "Info",
									MUIA_Frame, MUIV_Frame_Button,
									MUIA_InputMode    , MUIV_InputMode_RelVerify,
									MUIA_Background,  MUII_ButtonBack,
									MUIA_Text_SetMax, TRUE,
								End,
							End,
							Child, RegisterGroup(ppp),
								Child, VGroup,
									Child, data_tmp.filter_gui = MediaGetGuiTags(
										MGG_Type, MGG_Type_Filters,
										MGG_Media, MMT_VIDEO,
										MGG_Selector, MGG_Selector_List,
										MUIA_Frame, MUIV_Frame_Group,  
										TAG_END),
									Child, HGroup,
										Child, HVSpace,
										Child, data_tmp.filter_btn = TextObject,
												MUIA_Font, MUIV_Font_Button,
												MUIA_Text_Contents, "\33cFilter image",
												MUIA_Frame, MUIV_Frame_Button,
												MUIA_InputMode    , MUIV_InputMode_RelVerify,
												MUIA_Background,  MUII_ButtonBack,
												//MUIA_Text_SetMax, TRUE,
										End,
										Child, HVSpace,
									End,
								End,
								Child, VGroup,
									Child, data_tmp.saver_gui = MediaGetGuiTags(
										MGG_Type, MGG_Type_Muxers,
										MGG_Media, MMT_VIDEO,
										MGG_Selector, MGG_Selector_List,
										MGG_Selected, "png.muxer",
										MUIA_Frame, MUIV_Frame_Group,  
									TAG_END),
									Child,  HGroup,
										Child, data_tmp.save_btn = TextObject,
											MUIA_Font, MUIV_Font_Button,
											MUIA_Text_Contents, "Save",
											MUIA_Frame, MUIV_Frame_Button,
											MUIA_InputMode    , MUIV_InputMode_RelVerify,
											MUIA_Background,  MUII_ButtonBack,
											MUIA_Text_SetMax, TRUE,
										End,
										Child, PopaslObject,
											MUIA_Popasl_Type, ASL_FileRequest,
											ASLFR_TitleText, "Select image to save",
											MUIA_Popstring_Button, PopButton( MUII_PopFile ),
											MUIA_Popstring_String,  data_tmp.save_str =  StringObject,
												StringFrame,
												MUIA_String_MaxLen, 256,
												MUIA_CycleChain, 1,
												MUIA_String_AdvanceOnCR, TRUE,
											End,
										End,
									End,
								End,
							End, 
						End,			
					End,
					SubWindow, data_tmp.info_window  = WindowObject,
						MUIA_Window_Title, "Image Information",
						MUIA_Window_ID, MAKE_ID('I','N','F','O'),    
						MUIA_Window_Open, FALSE,  
							WindowContents, data_tmp.infomain = VGroup, 
								Child, data_tmp.info_group= ColGroup(2),
									
								End,
							End,
					End,
	TAG_MORE, msg->ops_AttrList))
	{		
				
		ProgressiveTester_Data   *data = INST_DATA(cl, obj);
		*data = data_tmp;
		// window closing
		DoMethod(data->window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,  obj, 2,
		         MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
		         
		// info window closing          
		DoMethod(data->info_window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,  data->info_window, 3,
		         MUIM_Set, MUIA_Window_Open, FALSE);     
		                          
        DoMethod(data->info_btn, MUIM_Notify, MUIA_Pressed, FALSE, data->info_window, 3, MUIM_Set, MUIA_Window_Open, TRUE);    
        
        DoMethod(data->uri, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, obj, 2, 
				MUIM_TesterLoadImage, MUIV_TriggerValue);

       	DoMethod(data->window, MUIM_Notify, MUIA_AppMessage, MUIV_EveryTime, MUIV_Notify_Application, 2, MUIM_TesterHandleAppMessage, MUIV_TriggerValue);
        DoMethod(data->save_btn, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_TesterSaveImage);
		DoMethod(data->filter_btn, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_TesterProcessImage);
		
		// open main window
		SetAttrs(data->window, MUIA_Window_Open, TRUE, TAG_END);
		
		return((IPTR)obj);
	}

	return(FALSE);
}

///

void Parse_Attributes(ProgressiveTester_Data  *data)
{    
	char buffer[1024];

    if (DoMethod(data->picture_reggae, MMM_GetPort, 0, MMA_Video_PaletteLength, &data->image_data.palette_len))
    {
        DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bPallette lenght:\33n"));
        sprintf(buffer, "%d", data->image_data.palette_len);
        DoMethod(data->info_group, MUIM_Group_AddTail, TextObject, MUIA_Text_Contents, buffer, End);
    }
    
    if (data->image_data.dpi_x)
    {
        DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bDPI X:\33n"));
        sprintf(buffer, "%d", data->image_data.dpi_x);
        DoMethod(data->info_group, MUIM_Group_AddTail, TextObject, MUIA_Text_Contents, buffer, End);
    }
    if (data->image_data.dpi_y)
    {
        DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bDPI Y:\33n"));
        sprintf(buffer, "%d", data->image_data.dpi_y);
        DoMethod(data->info_group, MUIM_Group_AddTail, TextObject, MUIA_Text_Contents, buffer, End);
    }
    
    if (data->image_data.aspect_x)
    {
        DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bAspect X:\33n"));
        sprintf(buffer, "%d", data->image_data.aspect_x);
        DoMethod(data->info_group, MUIM_Group_AddTail, TextObject, MUIA_Text_Contents, buffer, End);
    }	
    if (data->image_data.aspect_y)
    {
        DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bAspect Y:\33n"));
        sprintf(buffer, "%d", data->image_data.aspect_y);
        DoMethod(data->info_group, MUIM_Group_AddTail, TextObject, MUIA_Text_Contents, buffer, End);
    }	
    
    if (data->image_data.gpslat)
    {
        DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bLatitude:\33n"));
        sprintf(buffer, "%f", data->image_data.gpslat);
        DoMethod(data->info_group, MUIM_Group_AddTail, TextObject, MUIA_Text_Contents, buffer, End);
    }	
    
    if (data->image_data.gpslong)
    {
        DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bLongitude:\33n"));
        sprintf(buffer, "%f", data->image_data.gpslong);
        DoMethod(data->info_group, MUIM_Group_AddTail, TextObject, MUIA_Text_Contents, buffer, End);
    }	
    
    if (data->image_data.gpsalt)
    {
        DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bAltitude:\33n"));
        sprintf(buffer, "%f", data->image_data.gpsalt);
        DoMethod(data->info_group, MUIM_Group_AddTail, TextObject, MUIA_Text_Contents, buffer, End);
    }	
    
    
    if (data->image_data.progressive = MediaGetPort(data->picture_reggae, 0, MMA_Video_Progressive))
    {
        DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bProgressive:\33n"));
        DoMethod(data->info_group, MUIM_Group_AddTail, TextObject, MUIA_Text_Contents, "Yes", End);
        // turn on progressive decoding
        MediaSetPort(data->picture_reggae, 0, MMA_Video_Progressive, 1);
    }
    
    if (data->image_data.author[0] ) 
    {
        DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bAuthor:\33n"));
        DoMethod(data->info_group, MUIM_Group_AddTail, TextObject, MUIA_Text_Contents, data->image_data.author, End);
    }
     
    if (data->image_data.title[0] ) 
    {
        DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bTitle:\33n"));
        DoMethod(data->info_group, MUIM_Group_AddTail, TextObject, MUIA_Text_Contents, data->image_data.title, End);
    }
     
    if (data->image_data.copyright[0] ) 
    {
        DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bCopyright:\33n"));
        DoMethod(data->info_group, MUIM_Group_AddTail, TextObject, MUIA_Text_Contents, data->image_data.copyright, End);
    }
     
    if (data->image_data.comment[0] ) 
    {
        DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bComment:\33n"));
        DoMethod(data->info_group, MUIM_Group_AddTail, TextObject, MUIA_Text_Contents, data->image_data.comment, End);
    }
    if (data->image_data.cameravendor[0] ) 
    {
        DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bCamera vendor:\33n"));
        DoMethod(data->info_group, MUIM_Group_AddTail, TextObject, MUIA_Text_Contents, data->image_data.cameravendor, End);
    }
    if (data->image_data.cameramodel[0] ) 
    {
        DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bCamera model:\33n"));
        DoMethod(data->info_group, MUIM_Group_AddTail, TextObject, MUIA_Text_Contents, data->image_data.cameramodel, End);
    }
 
}

void Parse_Metadata(ProgressiveTester_Data  *data)
{
    LONG importance;
    double value;
    struct MetaPort icc_port;
    Object *filter = MediaGetPort(data->picture_reggae, 0, MMA_Port_Object);
   	struct TagItem *mainlist;
	double *p;
	ULONG query_tab[] = {MMETA_VideoDpiX, MMETA_VideoDpiY, MMETA_VideoAspectX, MMETA_VideoAspectY,  NULL};
	ULONG query_tab2[] = {MMETA_GPSLatitude, MMETA_GPSLongitude, MMETA_GPSAltitude,  NULL};
    value = 0;    
	
	if (mainlist = DoMethod(filter,  MMM_QueryMetaData, 1, query_tab, NULL))
	{
		p = (double*)GetTagData(MMDI_Data, 0, (struct TagItem*)GetTagData(MMETA_VideoDpiX, 0, mainlist));
		if (p) data->image_data.dpi_x  = *p;
		
		p = (double*)GetTagData(MMDI_Data, 0, (struct TagItem*)GetTagData(MMETA_VideoDpiY, 0, mainlist));
		if (p) data->image_data.dpi_y = *p;
		
		p = (double*)GetTagData(MMDI_Data, 0, (struct TagItem*)GetTagData(MMETA_VideoAspectX, 0, mainlist));
		if (p) data->image_data.aspect_x = *p;
		
		p = (double*)GetTagData(MMDI_Data, 0, (struct TagItem*)GetTagData(MMETA_VideoAspectY, 0, mainlist));
		if (p) data->image_data.aspect_y = *p;
		
		
		DoMethod(filter, MMM_DisposeMetaData, (IPTR)mainlist);	
	}


    DoMethod(filter, MMM_GetMetaItem, MMETA_Title,     &importance, &data->image_data.title,     MIBENUM_SYSTEM, 0);
    DoMethod(filter, MMM_GetMetaItem, MMETA_Author,    &importance, &data->image_data.author,    MIBENUM_SYSTEM, 0);
    DoMethod(filter, MMM_GetMetaItem, MMETA_Copyright, &importance, &data->image_data.copyright, MIBENUM_SYSTEM, 0);
    DoMethod(filter, MMM_GetMetaItem, MMETA_Comment,   &importance, &data->image_data.comment,   MIBENUM_SYSTEM, 0);
    DoMethod(filter, MMM_GetMetaItem, MMETA_CameraModel,  &importance, &data->image_data.cameramodel,    MIBENUM_SYSTEM, 0);
    DoMethod(filter, MMM_GetMetaItem, MMETA_CameraVendor, &importance, &data->image_data.cameravendor,   MIBENUM_SYSTEM, 0);
    
	if (mainlist = DoMethod(filter,  MMM_QueryMetaData, 1, query_tab2, NULL))
	{
		p = (double*)GetTagData(MMDI_Data, 0, (struct TagItem*)GetTagData(MMETA_GPSLatitude, 0, mainlist));
		if (p) data->image_data.gpslat = *p;
		
		p = (double*)GetTagData(MMDI_Data, 0, (struct TagItem*)GetTagData(MMETA_GPSLongitude, 0, mainlist));
		if (p) data->image_data.gpslong = *p;
		
		p = (double*)GetTagData(MMDI_Data, 0, (struct TagItem*)GetTagData(MMETA_GPSAltitude, 0, mainlist));
		if (p) data->image_data.gpsalt = *p;
				
		DoMethod(filter, MMM_DisposeMetaData, (IPTR)mainlist);	
	}
    
    if (DoMethod(filter, MMM_GetMetaItem, MMETA_ColorProfile, &importance, &icc_port, 0 , 0))
    {
    	ULONG total = (ULONG)MediaGetPort64(icc_port.mtp_Object , icc_port.mtp_Port, MMA_StreamLength);
    	D(KPrintF("ICC profile len=%d\n", total));
			
		if (data->image_data.icc) FreeVec(data->image_data.icc);
    	if((data->image_data.icc = AllocVec(total, MEMF_ANY)))
    	{
    		DoMethod(icc_port.mtp_Object, MMM_Pull, icc_port.mtp_Port, data->image_data.icc,  total);
    	}
    }
}

void Parse_Palette(ProgressiveTester_Data  *data)
{
    // get palette    
    if (data->image_data.palette_len>0 && data->image_data.palette_len<512)     
    {
        if (!DoMethod(data->picture_reggae, MMM_GetPort, 0, MMA_Video_Palette, &data->image_data.palette))
        {
            data->image_data.palette = NULL;
        }
        else if (data->image_data.palette)
        {   
            int col_i;
            Object *palette;
                  
            DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bPalette:\33n"));
            DoMethod(data->info_group, MUIM_Group_AddTail, palette=RowGroup(data->image_data.palette_len/16+1), End);
            DoMethod(palette, MUIM_Group_InitChange);
            for (col_i=0; col_i<data->image_data.palette_len; col_i++)
            {
                sprintf(color[col_i], "2:%02lX000000,%02lX000000,%02lX000000", (data->image_data.palette[col_i]&0xFF0000)>>16,
                                          (data->image_data.palette[col_i]&0xFF00)>>8,(data->image_data.palette[col_i]&0xFF) );
                DoMethod(palette, MUIM_Group_AddTail, RectangleObject,	MUIA_FixHeightTxt, "X", MUIA_Background, color[col_i], End);
            }
            DoMethod(palette, MUIM_Group_ExitChange);       
        }
    }
}


IPTR ProgressiveTester_ProcessImage(struct IClass *cl, Object *obj, struct MUIP_TesterLoad *msg)
{
	ProgressiveTester_Data  *data = INST_DATA(cl, obj);
	LONG ret = FALSE;
	UQUAD start_time, end_time;
	ULONG cpu_freq;
	double time_load;
	char info_line[1024];
	Object *saver, *output, *memory_stream, *video_filter, *filter_process;
	QUAD slen = data->image_header.Width*data->image_header.Height*4;
	
	set(data->text_log, MUIA_Text_Contents, "Processing...");
	set(data->window, MUIA_Window_Sleep, TRUE);
	cpu_freq = ReadCPUClock(&start_time);
	
	KPrintF("Before filtering: W: %d,  H: %d\n", data->image_header.Width, data->image_header.Height);
	
	if (memory_stream = NewObject(NULL, "memory.stream",
		MMA_StreamHandle, (IPTR)data->image_data.img+sizeof(struct RawImageHeader),
		MMA_StreamLength, (IPTR)&slen,
	TAG_END))
    {
		if (video_filter = NewObject(NULL, "rawvideo.filter",
			MMA_Video_Width, data->image_header.Width,
			MMA_Video_Height, data->image_header.Height,
			MMA_Port_Format, MMFC_VIDEO_ARGB32,
		TAG_END))
		{
			if (MediaConnectTagList(memory_stream, 0, video_filter, 0, NULL))
			{		
				if ((filter_process =   MediaBuildFromGuiTags(data->filter_gui, video_filter, 1, TAG_END)))
				{
				
					int h = MediaGetPort(filter_process, 1, MMA_Video_Height);
					int w = MediaGetPort(filter_process, 1, MMA_Video_Width);
					
					KPrintF("W: %d,  H: %d\n", w , h);
					
					if ((data->image_header.Height!=h) ||(data->image_header.Width!=w))
					{		
						KPrintF("Size changed\n");					
						data->image_header.Height = h;
						data->image_header.Width = w;
						
						if (data->image_data.img)
						{
							MediaFreeVec(data->image_data.img);
							data->image_data.img = NULL;
						}    
	   
						if(data->image_data.img = MediaAllocVec(data->image_header.Height * data->image_header.Width * 4+sizeof(struct RawImageHeader)))
						{
							CopyMem(&data->image_header, data->image_data.img, sizeof(data->image_header));
						}
						else 
							return FALSE;
					}

					
					DoMethod(filter_process, MMM_Pull, 1, (ULONG)(data->image_data.img+sizeof(struct RawImageHeader)), (int)w*h*4);
					DoMethod(data->maingroup, MUIM_Group_InitChange);
					if (data->image)
					{
						DoMethod(data->maingroup, MUIM_Group_Remove, data->image);
						MUI_DisposeObject(data->image);
					}
					data->image = RawimageObject, MUIA_Rawimage_Data, data->image_data.img, End;
					DoMethod(data->maingroup, MUIM_Family_Insert, data->image, data->area);
					
					DoMethod(data->maingroup, MUIM_Group_ExitChange);
					MUI_Redraw(data->image, MADF_DRAWOBJECT);
					
					DisposeObject(filter_process);
				}
				
			}
			DisposeObject(video_filter);
		}
		DisposeObject(memory_stream);
	}	
		
	set(data->window, MUIA_Window_Sleep, FALSE);
    ReadCPUClock(&end_time);
	end_time -= start_time;
	time_load = (double)end_time / cpu_freq;
	sprintf(info_line, "Image processed [%f seconds].",  time_load);
	set(data->text_log, MUIA_Text_Contents, info_line);
	
	return ret;
}

IPTR ProgressiveTester_SaveImage(struct IClass *cl, Object *obj, struct MUIP_TesterLoad *msg)
{
	ProgressiveTester_Data  *data = INST_DATA(cl, obj);
	LONG ret = FALSE;
	UQUAD start_time, end_time;
	ULONG cpu_freq;
	double time_load;
	char info_line[1024];
	Object *saver, *output, *memory_stream, *video_filter;
	QUAD slen = data->image_header.Width*data->image_header.Height*4;
	
	set(data->text_log, MUIA_Text_Contents, "Encoding...");
	set(data->window, MUIA_Window_Sleep, TRUE);
	cpu_freq = ReadCPUClock(&start_time);
	
	if (memory_stream = NewObject(NULL, "memory.stream",
		MMA_StreamHandle, (IPTR)data->image_data.img+sizeof(struct RawImageHeader),
		MMA_StreamLength, (IPTR)&slen,
	TAG_END))
    {
		if (video_filter = NewObject(NULL, "rawvideo.filter",
			MMA_Video_Width, data->image_header.Width,
			MMA_Video_Height, data->image_header.Height,
			MMA_Port_Format, MMFC_VIDEO_ARGB32,
		TAG_END))
		{
			if (MediaConnectTagList(memory_stream, 0, video_filter, 0, NULL))
			{		
				if ((saver =   MediaBuildFromGuiTags(data->saver_gui, video_filter, 1, TAG_END)))
				{
					char *str, *suffix = NULL;
					char tmp_name[1024];
					double metadata_double;
					
					// Storing metadata info
					if (data->image_data.dpi_x)
					{
						metadata_double = data->image_data.dpi_x;
						DoMethod(video_filter, MMM_AddMetaItem, 1, MMETA_VideoDpiX, MIMP_SECONDARY, &metadata_double, 0);
					}
					
					if (data->image_data.dpi_y)
					{
						metadata_double = data->image_data.dpi_y;
						DoMethod(video_filter, MMM_AddMetaItem, 1, MMETA_VideoDpiX, MIMP_SECONDARY, &metadata_double, 0);
					}
				
					if (data->image_data.aspect_x)
					{
						metadata_double = data->image_data.aspect_x;
						DoMethod(video_filter, MMM_AddMetaItem, 1, MMETA_VideoAspectX, MIMP_SECONDARY, &metadata_double, 0);
					}
					
					if (data->image_data.aspect_y)
					{
						metadata_double = data->image_data.aspect_y;
						DoMethod(video_filter, MMM_AddMetaItem, 1, MMETA_VideoAspectY, MIMP_SECONDARY, &metadata_double, 0);
					}
					
					// getting data
					get (data->save_str, MUIA_String_Contents, &str);
					// getting default suffix
					get (saver, MMA_DefaultExtension, &suffix);
					D(KPrintF("Suffix: %s\n", suffix));
					
					if (strlen(str))
					{   
                        if (suffix)
                            sprintf(tmp_name, "%s.%s", str,  suffix);
                        else
                            strcpy(tmp_name, str);
                        set (data->save_str, MUIA_String_Contents, tmp_name);
                        if (output = NewObject(NULL, "file.output",
                            MMA_StreamName, tmp_name, TAG_END))
                        {
                            if (MediaConnectTagList(saver, 1, output, 0, TAG_END))
                            {
                            
                                DoMethod(output, MMM_SignalAtEnd, (IPTR)FindTask(NULL), SIGBREAKB_CTRL_C);
                                DoMethod(output, MMM_Play);
                                Wait(SIGBREAKF_CTRL_C);
                                
                            }
                            DisposeObject(output);
                        }
					}
					DisposeObject(saver);
				}		
			}
			DisposeObject(video_filter);
		}
		DisposeObject(memory_stream);
	}
     
    set(data->window, MUIA_Window_Sleep, FALSE);
    ReadCPUClock(&end_time);
	end_time -= start_time;
	time_load = (double)end_time / cpu_freq;
	sprintf(info_line, "Image saved [%f seconds].",  time_load);
	set(data->text_log, MUIA_Text_Contents, info_line);
	
	return ret;
}


IPTR ProgressiveTester_LoadImage(struct IClass *cl, Object *obj, struct MUIP_TesterLoad *msg)
{
	ProgressiveTester_Data  *data = INST_DATA(cl, obj);
	LONG ret = FALSE;
	char buffer[1024];
	char info_line[1024];
	char *ptr;
	LONG reggae_error;
	UQUAD start_time, end_time;
	ULONG cpu_freq;
	double time_load;
	struct TagItem tags[10];
	
	tags[0].ti_Tag = MMA_StreamType;
	if (strstr(msg->filename, "http://"))
	{
		tags[0].ti_Data = (ULONG)"http.stream";
		tags[1].ti_Data = (ULONG)(msg->filename+7);
	}
	else
	{
		tags[0].ti_Data = (ULONG)"file.stream";
		tags[1].ti_Data = (ULONG)msg->filename;
	}

	tags[1].ti_Tag = MMA_StreamName;
	tags[2].ti_Tag = MMA_MediaType;
	tags[2].ti_Data = MMT_PICTURE,
	tags[3].ti_Tag = MMA_ErrorCode;
	tags[3].ti_Data = (ULONG)&reggae_error;
	tags[4].ti_Tag = TAG_END;
	tags[4].ti_Data = 0;
	
	data->image_data.dpi_x = data->image_data.dpi_y = data->image_data.aspect_y = data->image_data.aspect_x = 0;
    data->image_data.palette_len =0;
      
	if((data->picture_reggae = MediaNewObjectTagList(tags)))
	{
		data->image_header.Height = MediaGetPort(data->picture_reggae, 0, MMA_Video_Height);
		data->image_header.Width = MediaGetPort(data->picture_reggae, 0, MMA_Video_Width);
		data->image_header.Format = 0;
		data->image_header.Size = 0;
		
		DoMethod(data->infomain, MUIM_Group_InitChange);
		DoMethod(data->infomain, MUIM_Group_Remove, data->info_group);
		MUI_DisposeObject(data->info_group);
		data->info_group = ColGroup(2), End;
		DoMethod(data->infomain, MUIM_Group_AddTail, data->info_group);
		DoMethod(data->infomain, MUIM_Group_ExitChange);
		DoMethod(data->info_group, MUIM_Group_InitChange);
		DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bFilename:\33n"));
		DoMethod(data->info_group, MUIM_Group_AddTail, TextObject, MUIA_Text_Contents, msg->filename, End);
		DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bFormat:\33n"));
		DoMethod(data->info_group, MUIM_Group_AddTail, TextObject, MUIA_Text_Contents, MediaGetPort(data->picture_reggae, 0, MMA_DataFormat), End);
		DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bWidth:\33n"));
		sprintf(buffer, "%ld", data->image_header.Width);
		DoMethod(data->info_group, MUIM_Group_AddTail, TextObject, MUIA_Text_Contents, buffer, End);
		DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bHeight:\33n"));
		sprintf(buffer, "%ld", data->image_header.Height);
		DoMethod(data->info_group, MUIM_Group_AddTail, TextObject, MUIA_Text_Contents, buffer, End);
		DoMethod(data->info_group, MUIM_Group_AddTail, Label("\33bBits per pixel:\33n"));
		sprintf(buffer, "%ld", MediaGetPort(data->picture_reggae, 0, MMA_Video_BitsPerPixel));
		DoMethod(data->info_group, MUIM_Group_AddTail, TextObject, MUIA_Text_Contents, buffer, End);
		
	    Parse_Metadata(data);
	    Parse_Attributes(data);
        Parse_Palette(data);      
    
        DoMethod(data->info_group, MUIM_Group_ExitChange);
        
        if (data->image_data.img)
        {
            MediaFreeVec(data->image_data.img);
            data->image_data.img = NULL;
        }    
        // read image bitmap
		if(data->image_data.img = MediaAllocVec(data->image_header.Height * data->image_header.Width * 4+sizeof(struct RawImageHeader)))
		{
			BOOL  is_final_touch = FALSE;			
			ULONG bytes_read = 0, total_bytes_read = 0;
			BOOL  read_error     = FALSE;
			int   read_mode      = 0;
			ULONG   lines;
			 
			ptr = data->image_data.img+sizeof(struct RawImageHeader);

			CopyMem(&data->image_header, data->image_data.img, sizeof(data->image_header));
			while(!is_final_touch)
			{
				set(data->text_log, MUIA_Text_Contents, "Decoding...");
				get(data->mode_radio, MUIA_Radio_Active,  &read_mode);
				
				cpu_freq = ReadCPUClock(&start_time);
				
				switch (read_mode)
				{
						// full image
						case MODE_Full_Image:
							total_bytes_read = DoMethod(data->picture_reggae, MMM_Pull, 0, ptr, data->image_header.Height * data->image_header.Width * 4);
				
							if (total_bytes_read != (data->image_header.Height * data->image_header.Width * 4))
							{
								memset(ptr+total_bytes_read, 0, data->image_header.Height * data->image_header.Width * 4-total_bytes_read);
								read_error     = TRUE;
							}
						break;
						// full image, line by line
						case MODE_Line_Per_Line:
							for (lines=0; lines<data->image_header.Height; lines++)
							{
								bytes_read = DoMethod(data->picture_reggae, MMM_Pull, 0, ptr+total_bytes_read, data->image_header.Width * 4);
								total_bytes_read += bytes_read;
								if (bytes_read != (data->image_header.Width * 4))
								{
									memset(ptr+bytes_read, 0, data->image_header.Height * data->image_header.Width * 4-bytes_read);
									read_error     = TRUE;
									break;
								}
							}
						
						break;
						
						// a few bytes from 1st line and other bytes read full width per Pull()
						case MODE_Combined:
							bytes_read = DoMethod(data->picture_reggae, MMM_Pull, 0, ptr+total_bytes_read, 8 * 4);
							total_bytes_read += bytes_read;
							D(KPrintF("Bytes read %d, total bytes: %d\n", bytes_read, total_bytes_read));
							for (lines=0; lines<data->image_header.Height-1; lines++)
							{
								bytes_read = DoMethod(data->picture_reggae, MMM_Pull, 0, ptr+total_bytes_read, data->image_header.Width * 4);
								total_bytes_read += bytes_read;
								D(KPrintF("Bytes read %d, total bytes: %d\n", bytes_read, total_bytes_read ));
								if (bytes_read != (data->image_header.Width * 4))
								{
									memset(ptr+total_bytes_read, 0, data->image_header.Height * data->image_header.Width * 4-total_bytes_read);
									read_error     = TRUE;
									break;
								}
							}
							
							bytes_read = DoMethod(data->picture_reggae, MMM_Pull, 0, ptr+total_bytes_read, (data->image_header.Width-10) * 4);
							total_bytes_read += bytes_read;
							D(KPrintF("Bytes read %d, total bytes: %d\n", bytes_read, total_bytes_read));
						break;
						
						// half image
						case MODE_Half_Image:
							total_bytes_read = DoMethod(data->picture_reggae, MMM_Pull, 0, ptr, data->image_header.Height * data->image_header.Width * 4/2);
				
							if (total_bytes_read != (data->image_header.Height * data->image_header.Width * 4/2))
							{			
								read_error     = TRUE;
							}
							memset(ptr+total_bytes_read, 0, data->image_header.Height * data->image_header.Width * 4-total_bytes_read);
						
						break;
						
						
						get(data->picture_reggae,  MMA_ErrorCode, &reggae_error);
						if (reggae_error)
                            break;
						
				}
				
				ReadCPUClock(&end_time);
				time_load = (double)(end_time-start_time) / cpu_freq;

				DoMethod(data->maingroup, MUIM_Group_InitChange);
				if (data->image)
				{
					DoMethod(data->maingroup, MUIM_Group_Remove, data->image);
					MUI_DisposeObject(data->image);
				}
				data->image = RawimageObject, MUIA_Rawimage_Data, data->image_data.img, End;
				DoMethod(data->maingroup, MUIM_Family_Insert, data->image, data->area);
				
				DoMethod(data->maingroup, MUIM_Group_ExitChange);
				MUI_Redraw(data->image, MADF_DRAWOBJECT);
			
				is_final_touch = MediaGetPort(data->picture_reggae, 0, MMA_Video_FinalTouch) || !data->image_data.progressive; 
			}
			if (read_error == TRUE)
			{
				sprintf(info_line, "Image loaded with errors (only got %ld bytes).\n", bytes_read);
				set(data->text_log, MUIA_Text_Contents, info_line);
			}
			else
			{
				sprintf(info_line, "Image loaded [%f seconds].",  time_load);
				set(data->text_log, MUIA_Text_Contents, info_line);
			}	
		}
		else
		{
			set(data->text_log, MUIA_Text_Contents, "Cannot allocate memory.");
		}

		if(data->picture_reggae != NULL)
		{
			DisposeObject(data->picture_reggae);
			data->picture_reggae = NULL;
		}

		ret = TRUE;
	}
	else
	{
		sprintf(data->error_line,"Unable to load image \33b%s\33n: %s.", msg->filename,  MediaFault(reggae_error));
		set(data->text_log, MUIA_Text_Contents, data->error_line);
	}
	return ret;
}

///
/// ProgressiveTester_Dispose()

ULONG ProgressiveTester_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
	ProgressiveTester_Data *data;

	data = INST_DATA(cl, obj);

    if (data->image_data.img)
    {
        MediaFreeVec(data->image_data.img);
    }    
    
    if (data->image_data.icc)
    {
    	FreeVec(data->image_data.icc);
    }
	SetAttrs(data->window, MUIA_Window_Open, FALSE, TAG_END);

	return(DoSuperMethodA(cl, obj, msg));
}

///


ULONG ProgressiveTester_HandleAppMessage(struct IClass *cl, Object *obj, struct MUIP_TesterHandleAppMessage *msg)
{
    ProgressiveTester_Data *data;

	struct AppMessage *amsg = msg->appmessage;
	struct WBArg *ap = amsg->am_ArgList;
	char buf[1024];

	NameFromLock(ap->wa_Lock, buf,sizeof(buf));
	AddPart(buf,ap->wa_Name,  sizeof(buf));
	data = INST_DATA(cl, obj);
    set(data->uri, MUIA_String_Contents, buf);
	DoMethod(obj, MUIM_TesterLoadImage, buf);

	return 0;
}

/// ProgressiveTester_Dispatcher()

DISPATCHER(ProgressiveTester_Class_)
	switch(msg->MethodID)
	{
		case OM_NEW:		         return (ProgressiveTester_New(cl, obj, (struct opSet*)msg));
		case OM_DISPOSE:	         return (ProgressiveTester_Dispose(cl, obj, (APTR)msg));
		case MUIM_TesterLoadImage:   return (ProgressiveTester_LoadImage(cl, obj, (struct MUIP_TesterLoad*)msg));
		case MUIM_TesterSaveImage:   return (ProgressiveTester_SaveImage(cl, obj, (struct MUIP_TesterLoad*)msg));
		case MUIM_TesterProcessImage: return (ProgressiveTester_ProcessImage(cl, obj, (struct MUIP_TesterLoad*)msg));
		case MUIM_TesterHandleAppMessage: return(ProgressiveTester_HandleAppMessage(cl, obj, (struct MUIP_TesterHandleAppMessage*)msg));
		
		default:			return (DoSuperMethodA(cl, obj, msg));
	}
DISPATCHER_END

///
/// Main()

struct Library *RawFilterBase;
struct Library *MemoryStreamBase;
struct Library *FileOutputBase;

int main(int argc, char **argv)
{
	ULONG signals;
	Object *app;
	
	if(argc == 2)
	{
		filename = argv[1];
	}

	if ((RawFilterBase = OpenLibrary("multimedia/rawvideo.filter", 51)))
	{
        if ((MemoryStreamBase = OpenLibrary("multimedia/memory.stream", 51)))
        {
            if ((FileOutputBase = OpenLibrary("multimedia/file.output", 51)))
            {
                if(!(ProgressiveTesterClass = CreateProgressiveTesterClass()))
                {
                    Printf("Failed to create custom class.\n");
                }
                else
                {
                    if((app = NewObject(ProgressiveTesterClass->mcc_Class, NULL, TAG_DONE)))
                    {
                        while(DoMethod(app, MUIM_Application_NewInput, &signals) != MUIV_Application_ReturnID_Quit)
                        {
                            if(signals)
                            {
                                signals = Wait(signals | SIGBREAKF_CTRL_C);

                                if(signals & SIGBREAKF_CTRL_C)
                                    break;
                            }
                        }

                        // dispose the application object
                        MUI_DisposeObject(app);
                    }
                    else
                        Printf("Failed to create Application.\n");

                    MUI_DeleteCustomClass(ProgressiveTesterClass);
                }
                CloseLibrary(FileOutputBase);
            }
            CloseLibrary(RawFilterBase);
        }
        CloseLibrary(MemoryStreamBase);
    }
   
	return(0);
}

///
