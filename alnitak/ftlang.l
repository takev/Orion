
%{

%}


%option noyywrap
%x LANG STRING
%%


<INITIAL>^\ *%                              BEGIN(LANG); yyless(1);
<INITIAL>\${                                BEGIN(LANG); return T_PLACEHOLDER;
<INITIAL>[^%$]+
<INITIAL>\\\n                               yyless(2);
<INITIAL>.                                  yymore();

<LANG>[a-zA-Z_][a-zA-Z0-9_]+                return T_NAME;
<LANG>-?[0-9]+\.[0-9]+                      return T_FLOAT;
<LANG>-?[0-9]+                              return T_INTEGER;
<LANG>\"                                    BEGIN(STRING);
<STRING>[^"]*                               yymore();
<STRING>\\\"                                yyless(1);
<STRING>\"                                  BEGIN(LANG); return T_STRING;
<LANG>\*\*                                  return T_POWER;
<LANG>==                                    return T_EQ;
<LANG>!=                                    return T_NE;
<LANG><=                                    return T_LE;
<LANG>>=                                    return T_GE;
<LANG><<                                    return T_SHIFT_LEFT;
<LANG>>>                                    return T_SHIFT_RIGHT;
<LANG>in                                    return T_IN;
<LANG>[-+*/%~!&|().,^<>=]                   return *yytext;

<LANG>%if                                   return T_IF;
<LANG>%elsif                                return T_ELSIF;
<LANG>%else                                 return T_ELSE;
<LANG>%for                                  return T_FOR;
<LANG>%let                                  return T_LET;
<LANG>%block                                return T_BLOCK;
<LANG>%function                             return T_FUNCTION;
<LANG>%end                                  return T_END;

<LANG>}                                     BEGIN(INITIAL); return T_END_PLACEHOLDER;
<LANG>:?\r?\n                               BEGIN(INITIAL); return T_NL;

%%

