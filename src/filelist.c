#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <filelist.h>
#include <toss.h>
#include <log.h>
#include <global.h>
#include <string.h>
#include <fcommon.h>
#include <common.h>
#include <progprot.h>
#include <add_descr.h>
#include <dir.h>

unsigned long totalfilessize = 0;
unsigned int totalfilesnumber = 0;

void formatDesc(char **desc, int *count, FILE *f)
//Don't work. :-((
//Why?
{
    int i;
    char **newDesc = 0, tmp[265], tmpDesc[265], *ch, buff[94], *tmp1;

   for (i = 0; i < (*count); i++ ) {
      newDesc=realloc(newDesc,(i+1)*sizeof(*newDesc));
      memset(buff, 0, sizeof buff);
      if (strlen(desc[i]) <= 46) {
	 newDesc[i] = strdup(desc[i]);
	 continue;
      }
      strcpy(tmp,desc[i]);

      ch = strtok(tmp, " \t\0");
      if (strlen(ch)>46) {
	 newDesc[i] = strdup(ch);
	 ch = strtok(NULL,"\0");
	 if (ch != NULL) {
            if ((*count) == i+1) {
               desc=realloc(desc,(*count)+1*sizeof(*desc));
	       (*count)++;
	       desc[i+1] = strdup(ch);
	    } else {
	       strcpy(tmpDesc,desc[i+1]);
               desc[i+1] = (char *) realloc(desc[i+1],strlen(desc[i+1])+strlen(ch)+1);
               sprintf(desc[i+1],"%s %s",ch,tmpDesc);
            }
	 }
	 continue;
      }
      while (ch != NULL) {
         if (strlen(buff)+strlen(ch)>46) {
	    newDesc[i] = strdup(buff);
	    if ((*count) == i+1) {
	       desc=realloc(desc,(*count)+1*sizeof(*desc));
	       (*count)++;
	       desc[i+1] = strdup(ch);
	       ch = strtok(NULL,"\0");
	       if (ch != NULL) {
	          strcpy(tmpDesc,desc[i+1]);
                  desc[i+1] = (char *) realloc(desc[i+1],strlen(desc[i+1])+strlen(ch)+1);
                  sprintf(desc[i+1],"%s %s",tmpDesc,ch);
	       }
	    } else {
	       strcpy(tmpDesc,desc[i+1]);
	       tmp1 = strtok(NULL,"\0");
	       if (tmp1 == NULL) {
                  desc[i+1] = (char *) realloc(desc[i+1],strlen(desc[i+1])+strlen(ch)+1);
                  sprintf(desc[i+1],"%s %s",ch,tmpDesc);
	       } else {
                  desc[i+1] = (char *) realloc(desc[i+1],strlen(desc[i+1])+strlen(ch)+strlen(tmp1)+2);
                  sprintf(desc[i+1],"%s %s %s",ch,tmp1,tmpDesc);
	       }
            }
	    ch = NULL;
	    continue;
	 }
         if (buff[0] == 0) sprintf(buff, "%s", ch);
         else sprintf(buff+strlen(buff), " %s", ch);
         ch = strtok(NULL, " \t\0");
      }
   }
   for (i = 0; i < (*count); i++ ) {
      free(desc[i]);
      desc[i] = strdup(newDesc[i]);
      free(newDesc[i]);
   }
   if ((*count) > 0) free(newDesc);
   return;
}

void putFileInFilelist(FILE *f, char *filename, off_t size, int day, int month, int year, int countdesc, char **desc)
{
    int i;

   fprintf(f,"%-12s",filename);
   fprintf(f,"%8lu ",size);
   fprintf(f, "%02u-", day);
   switch (month) {
   case 0: fprintf(f, "Jan");
      break;
   case 1: fprintf(f, "Feb");
      break;
   case 2: fprintf(f, "Mar");
      break;
   case 3: fprintf(f, "Apr");
      break;
   case 4: fprintf(f, "May");
      break;
   case 5: fprintf(f, "Jun");
      break;
   case 6: fprintf(f, "Jul");
      break;
   case 7: fprintf(f, "Aug");
      break;
   case 8: fprintf(f, "Sep");
      break;
   case 9: fprintf(f, "Oct");
      break;
   case 10: fprintf(f, "Nov");
      break;
   case 11: fprintf(f, "Dec");
      break;
   default:
      break;
   }
   fprintf(f, "-%02u", year % 100);
   if (countdesc == 0) fprintf(f," Description not avaliable\n");
   else {
      //formatDesc(desc, &countdesc, f);
      for (i=0;i<countdesc;i++) {
         if (i == 0) fprintf(f," %s\n",desc[i]);
	 else fprintf(f,"                               %s\n",desc[i]);
      }
   }
   return;
}

void printFileArea(char *area_areaName, char *area_pathName, char *area_description, FILE *f, int bbs) {

    char fileareapath[256], fbbsname[256], filename[256];
    char fbbsline[265];
    DIR            *dir;
    struct dirent  *file;
    s_ticfile tic;
    struct stat stbuf;
    time_t fileTime;
    struct tm *locTime;
    unsigned int totalnumber = 0;
    unsigned long totalsize = 0;
    char **removeFiles = NULL;
    unsigned int removeCount = 0, i, Len;
    FILE *fbbs;
    char *token = "";
    int flag = 0;

   strcpy(fileareapath,area_pathName);
   strLower(fileareapath);
   createDirectoryTree(fileareapath);
   strcpy(fbbsname,fileareapath);
   strcat(fbbsname,"files.bbs");

   dir = opendir(fileareapath);
   if (dir == NULL) return;

   while ((file = readdir(dir)) != NULL) {
      if (strcmp(file->d_name,".") == 0 || strcmp(file->d_name,"..") == 0) continue;
      strcpy(filename, fileareapath);
      strcat(filename, file->d_name);
      if (stricmp(filename, fbbsname) == 0) continue;
      else if (!flag) {
         if (bbs) fprintf(f,"BbsArea: %s", area_areaName);
         else fprintf(f,"FileArea: %s", area_areaName);
         if (area_description!=NULL)
            fprintf(f," (%s)\n", area_description);
          else
            fprintf(f,"\n");
          fprintf(f,"-----------------------------------------------------------------------------\n");
	  flag = 1;
      }
      memset(&tic,0,sizeof(tic));
      if (getDesc(fbbsname, file->d_name, &tic) == 1) {
         tic.desc=realloc(tic.desc,(tic.anzdesc+1)*sizeof(*tic.desc));
         tic.desc[tic.anzdesc]=strdup("Description not avaliable");
	 tic.anzdesc = 1;
         add_description(fbbsname, file->d_name, tic.desc, 1);
      }
      stat(filename,&stbuf);
      fileTime = stbuf.st_mtime;
      locTime = localtime(&fileTime);
      totalsize += stbuf.st_size;
      totalnumber++;
      putFileInFilelist(f, file->d_name, stbuf.st_size, locTime->tm_mday, locTime->tm_mon, locTime->tm_year, tic.anzdesc, tic.desc);
      disposeTic(&tic);
   }
   if (flag) {
      fprintf(f,"-----------------------------------------------------------------------------\n");
      fprintf(f,"Total files in area: %4d, total size: %10lu bytes\n\n",totalnumber,totalsize);
   }
   if ( (fbbs = fopen(fbbsname,"r")) == NULL ) return;
   while (!feof(fbbs)) {
         fgets(fbbsline,sizeof fbbsline,fbbs);

         if (fbbsline[0]==0 || fbbsline[0]==10 || fbbsline[0]==13
	     || fbbsline[0]==' ' || fbbsline[0]=='\t' || fbbsline[0]=='>')
            continue;

         Len = strlen(fbbsline);
         if (fbbsline[Len-1]=='\n')
            fbbsline[--Len]=0;

         if (fbbsline[Len-1]=='\r')
            fbbsline[--Len]=0;

         token = strtok(fbbsline, " \t\0");

         if (token==NULL)
            continue;

	 strLower(token);
         strcpy(filename,fileareapath);
         strcat(filename,token);
	 if (!fexist(filename)) {
            removeFiles = realloc(removeFiles, sizeof(char *)*(removeCount+1));
	    removeFiles[removeCount] = (char *) malloc(strlen(strrchr(filename,PATH_DELIM)+1)+1);
            strcpy(removeFiles[removeCount], strrchr(filename,PATH_DELIM)+1);
	    removeCount++;
	 }
	 fbbsline[0] = '\0';
   }
   fclose(fbbs);
   if (removeCount > 0) {
      for (i=0; i<removeCount; i++) {
        removeDesc(fbbsname,removeFiles[i]);
	free(removeFiles[i]);
      }
      free(removeFiles);
   }
   totalfilessize += totalsize;
   totalfilesnumber += totalnumber;
   return;
}

void filelist()
{
    FILE *f;
    char logstr[200];
    int i;

   if (strlen(flistfile) == 0) {
      writeLogEntry(htick_log,'6',"Not found output file");
      return;
   }

   if ( (f = fopen(flistfile,"w")) == NULL ) {
         sprintf(logstr,"Could not open for write file %s",flistfile);
         writeLogEntry(htick_log,'6',logstr);
         return;
   }

   for (i=0; i<config->fileAreaCount; i++) {
      if (config->fileAreas[i].pass != 1)
         printFileArea(config->fileAreas[i].areaName, config->fileAreas[i].pathName, config->fileAreas[i].description, f, 0);
   }

   for (i=0; i<config->bbsAreaCount; i++) {
      printFileArea(config->bbsAreas[i].areaName, config->bbsAreas[i].pathName, config->bbsAreas[i].description, f, 1);
   }

   fprintf(f,"=============================================================================\n");
   fprintf(f,"Total files in filelist: %4d, total size: %10lu bytes\n",totalfilesnumber,totalfilessize);
   fprintf(f,"=============================================================================\n\n");

   fclose(f);
}
