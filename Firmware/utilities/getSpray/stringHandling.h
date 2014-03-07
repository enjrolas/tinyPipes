uint16_t getIndex(char * buffer, char target)
{
  uint16_t pointer=0;
  while((*(buffer+pointer)!=target)&&(*(buffer+pointer)!=0))    //note -- this is vulnerable to any strings that aren't null-terminated
    pointer++;
  return pointer;
}


uint16_t getIndex(char * buffer,  char target, uint16_t start)
{
  uint16_t pointer=start;
  while((*(buffer+pointer)!=target)&&(*(buffer+pointer)!=0))    //note -- this is vulnerable to any strings that aren't null-terminated
    pointer++;
  return pointer;
}

void substring(char* source, char * destination, uint16_t from, uint16_t to)
{
  uint16_t pointer;
  for(pointer=from;pointer<to;pointer++)
    *(destination+pointer-from)=*(source + pointer);
  *(destination + pointer - from) =0;  //null-terminate the string
}

uint16_t horribleReadline(char *buffer, uint16_t limit)
{
  char a;
  uint16_t pointer=0;
  while(true)
  {
    if(GSM.available()>0){
      a=GSM.read();
      if(a==10) //check for a carriage return
      {
        *(buffer + pointer)=0;  //end the line
        return pointer;
      }
      else if(pointer<limit-1)
      {
        *(buffer + pointer) = a;
        pointer++;
      }
      else
      {
        return pointer;  //we hit the limit, we gotta call it
      }
    }
  }
}

void removeChar(char * buffer, char target)
{
  uint16_t pointer, runner, deleted;
  uint16_t limit=getLength(buffer);
  deleted=0;
  for(pointer=0;pointer<limit;pointer++)
  {   
    if(*(buffer+pointer+deleted)==target)
      deleted++;
    if((pointer+deleted)>limit)
    {
      *(buffer+pointer)=0;
      break;
    }
    else
      *(buffer+pointer)=*(buffer+pointer+deleted);
  }  
}

void trim(char * string, int length)
{
  uint16_t i=0; 
  int leading, trailing;
  while(isSpace(string[i])&&i<length)
    i++;
  leading=i;
  i=length-1;
  while(isSpace(string[i])&&(i>=0))
    i--;
  trailing=length-i-1;
  memcpy(string, string+leading, length-leading); //scoot everything past the leading whitespace
  for(i=length-leading-trailing;i<length;i++)  //and now white out the characters that we left at the end of the string
    *(string+i)=0;
}


boolean isSpace(char a)
{
  char spaces[4]={32, 10, 13, 9};
  boolean space=false;
  for(unsigned char i=0;i<strlen(spaces);i++)
    if(a==spaces[i])
      space=true;
  return space;
}

