#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
	NORMAL,
	IN_STRING,
	IN_TEMPLATE,
	IN_REGEX,
	IN_LINE_COMMENT,
	IN_BLOCK_COMMENT,
	IN_TAG,
	IN_SCRIPT,
	IN_STYLE,
	IN_TEXT
} State;

typedef struct {
	State state;
	State prev_state;
	int indent_level;
	char quote_char;
	int at_line_start;
	char tag_buffer[256];
	int tag_pos;
	int in_script_block;
	int in_style_block;
	char prev_char;
	int tag_started;
	int template_depth;
	char lookahead[16];
	int lookahead_pos;
	int brace_depth;
} Context;

void init_context(Context *ctx) {
	ctx->state = NORMAL;
	ctx->prev_state = NORMAL;
	ctx->indent_level = 0;
	ctx->quote_char = 0;
	ctx->at_line_start = 1;
	ctx->tag_pos = 0;
	ctx->in_script_block = 0;
	ctx->in_style_block = 0;
	ctx->prev_char = 0;
	ctx->tag_started = 0;
	ctx->template_depth = 0;
	ctx->lookahead_pos = 0;
	ctx->brace_depth = 0;
	memset(ctx->tag_buffer, 0, sizeof(ctx->tag_buffer));
	memset(ctx->lookahead, 0, sizeof(ctx->lookahead));
}

void write_indent(FILE *out, Context *ctx) {
	for (int i = 0; i < ctx->indent_level; i++) {
		fputc('\t', out);
	}
	ctx->at_line_start = 0;
}

void write_newline(FILE *out, Context *ctx) {
	fputc('\n', out);
	ctx->at_line_start = 1;
}

int is_self_closing(const char *tag) {
	const char *self_closing[] = {"br", "hr", "img", "input", "meta", "link", "area", "base", "col", "embed", "param", "source", "track", "wbr", NULL};
	for (int i = 0; self_closing[i]; i++) {
		if (strcasecmp(tag, self_closing[i]) == 0) return 1;
	}
	return 0;
}

int can_start_regex(char c) {
	return (c == '=' || c == '(' || c == '[' || c == ',' || c == ':' || 
	        c == '!' || c == '&' || c == '|' || c == '?' || c == '{' || 
	        c == ';' || c == '\n' || c == '\t' || c == ' ');
}

void handle_tag_end(FILE *out, Context *ctx) {
	if (ctx->tag_pos == 0) return;
	
	ctx->tag_buffer[ctx->tag_pos] = '\0';
	char *tag_name = ctx->tag_buffer;
	int is_closing = 0;
	
	if (tag_name[0] == '/') {
		is_closing = 1;
		tag_name++;
	}
	
	char clean_tag[256] = {0};
	int j = 0;
	for (int i = 0; tag_name[i] && !isspace(tag_name[i]); i++) {
		clean_tag[j++] = tolower(tag_name[i]);
	}
	clean_tag[j] = '\0';
	
	if (is_closing) {
		if (strcasecmp(clean_tag, "script") == 0) {
			ctx->in_script_block = 0;
			ctx->state = NORMAL;
		} else if (strcasecmp(clean_tag, "style") == 0) {
			ctx->in_style_block = 0;
			ctx->state = NORMAL;
		}
	} else {
		if (strcasecmp(clean_tag, "script") == 0) {
			ctx->in_script_block = 1;
		} else if (strcasecmp(clean_tag, "style") == 0) {
			ctx->in_style_block = 1;
		}
		if (!is_self_closing(clean_tag) && ctx->tag_buffer[ctx->tag_pos - 1] != '/') {
			ctx->indent_level++;
		}
	}
	
	ctx->tag_pos = 0;
	ctx->tag_started = 0;
	memset(ctx->tag_buffer, 0, sizeof(ctx->tag_buffer));
}

void process_char(FILE *out, Context *ctx, int c) {
	if (ctx->state == IN_REGEX) {
		fputc(c, out);
		if (c == '/' && ctx->prev_char != '\\') {
			ctx->state = ctx->in_script_block ? IN_SCRIPT : (ctx->in_style_block ? IN_STYLE : NORMAL);
		} else if (c == '\n') {
			ctx->state = ctx->in_script_block ? IN_SCRIPT : (ctx->in_style_block ? IN_STYLE : NORMAL);
			ctx->at_line_start = 1;
		}
		ctx->prev_char = c;
		return;
	}
	
	if (ctx->state == IN_TEMPLATE) {
		fputc(c, out);
		if (c == '`' && ctx->prev_char != '\\') {
			ctx->state = ctx->prev_state;
		} else if (c == '{' && ctx->prev_char == '$') {
			ctx->template_depth++;
		} else if (c == '}' && ctx->template_depth > 0) {
			ctx->template_depth--;
		}
		ctx->prev_char = c;
		return;
	}
	
	if (ctx->state == IN_STRING) {
		fputc(c, out);
		if (c == ctx->quote_char && ctx->prev_char != '\\') {
			ctx->state = ctx->prev_state;
		}
		ctx->prev_char = c;
		return;
	}
	
	if (ctx->state == IN_LINE_COMMENT) {
		fputc(c, out);
		if (c == '\n') {
			ctx->state = ctx->in_script_block ? IN_SCRIPT : (ctx->in_style_block ? IN_STYLE : NORMAL);
			ctx->at_line_start = 1;
		}
		ctx->prev_char = c;
		return;
	}
	
	if (ctx->state == IN_BLOCK_COMMENT) {
		fputc(c, out);
		if (c == '/' && ctx->prev_char == '*') {
			ctx->state = ctx->in_script_block ? IN_SCRIPT : (ctx->in_style_block ? IN_STYLE : NORMAL);
		}
		ctx->prev_char = c;
		return;
	}
	
	if (ctx->state == IN_TAG) {
		if (!ctx->tag_started) {
			ctx->tag_started = 1;
			if (c == '/') {
				ctx->indent_level--;
				if (ctx->indent_level < 0) ctx->indent_level = 0;
			}
			write_indent(out, ctx);
			fputc('<', out);
		}
		
		if (c == '\'' || c == '"') {
			ctx->quote_char = c;
			ctx->prev_state = IN_TAG;
			ctx->state = IN_STRING;
			fputc(c, out);
			ctx->prev_char = c;
			return;
		}
		
		if (c == '>') {
			fputc(c, out);
			handle_tag_end(out, ctx);
			if (ctx->in_script_block) {
				ctx->state = IN_SCRIPT;
				write_newline(out, ctx);
			} else if (ctx->in_style_block) {
				ctx->state = IN_STYLE;
				write_newline(out, ctx);
			} else {
				ctx->state = IN_TEXT;
			}
		} else {
			if (ctx->tag_pos < 255) {
				ctx->tag_buffer[ctx->tag_pos++] = c;
			}
			fputc(c, out);
		}
		ctx->prev_char = c;
		return;
	}
	
	if (ctx->state == IN_TEXT) {
		if (c == '<') {
			write_newline(out, ctx);
			ctx->state = IN_TAG;
			ctx->tag_pos = 0;
			ctx->tag_started = 0;
			ctx->prev_char = c;
			return;
		}
		
		if (isspace(c)) {
			if (!isspace(ctx->prev_char)) {
				fputc(' ', out);
			}
		} else {
			fputc(c, out);
		}
		
		ctx->prev_char = c;
		return;
	}
	
	if (ctx->state == IN_SCRIPT || ctx->state == IN_STYLE) {
		if (c == '<') {
			ctx->lookahead[0] = '<';
			ctx->lookahead_pos = 1;
		} else if (ctx->lookahead_pos > 0 && ctx->lookahead_pos < 15) {
			ctx->lookahead[ctx->lookahead_pos++] = c;
			ctx->lookahead[ctx->lookahead_pos] = '\0';
			
			if (strncasecmp(ctx->lookahead, "</script>", ctx->lookahead_pos) == 0 && 
			    ctx->lookahead_pos == 9 && ctx->in_script_block) {
				for (int i = 0; i < ctx->lookahead_pos - 1; i++) {
					fputc(ctx->lookahead[i], out);
				}
				ctx->lookahead_pos = 0;
				ctx->state = IN_TAG;
				ctx->tag_pos = 0;
				ctx->tag_started = 0;
				ctx->prev_char = '<';
				return;
			} else if (strncasecmp(ctx->lookahead, "</style>", ctx->lookahead_pos) == 0 && 
			           ctx->lookahead_pos == 8 && ctx->in_style_block) {
				for (int i = 0; i < ctx->lookahead_pos - 1; i++) {
					fputc(ctx->lookahead[i], out);
				}
				ctx->lookahead_pos = 0;
				ctx->state = IN_TAG;
				ctx->tag_pos = 0;
				ctx->tag_started = 0;
				ctx->prev_char = '<';
				return;
			} else if (ctx->lookahead_pos > 2 && ctx->lookahead[1] != '/') {
				for (int i = 0; i < ctx->lookahead_pos; i++) {
					fputc(ctx->lookahead[i], out);
				}
				ctx->lookahead_pos = 0;
				ctx->prev_char = c;
				return;
			}
			ctx->prev_char = c;
			return;
		} else if (ctx->lookahead_pos > 0) {
			for (int i = 0; i < ctx->lookahead_pos; i++) {
				fputc(ctx->lookahead[i], out);
			}
			ctx->lookahead_pos = 0;
		}
		
		if (c == '`') {
			if (ctx->at_line_start) write_indent(out, ctx);
			ctx->prev_state = ctx->state;
			ctx->state = IN_TEMPLATE;
			ctx->template_depth = 0;
			fputc(c, out);
			ctx->prev_char = c;
			return;
		}
		
		if ((c == '\'' || c == '"') && ctx->prev_char != '\\') {
			if (ctx->at_line_start) write_indent(out, ctx);
			ctx->quote_char = c;
			ctx->prev_state = ctx->state;
			ctx->state = IN_STRING;
			fputc(c, out);
			ctx->prev_char = c;
			return;
		}
		
		if (c == '/' && ctx->prev_char == '/') {
			ctx->state = IN_LINE_COMMENT;
			fputc(c, out);
			ctx->prev_char = c;
			return;
		}
		
		if (c == '*' && ctx->prev_char == '/') {
			ctx->state = IN_BLOCK_COMMENT;
			fputc(c, out);
			ctx->prev_char = c;
			return;
		}
		
		if (c == '/' && can_start_regex(ctx->prev_char)) {
			if (ctx->at_line_start) write_indent(out, ctx);
			ctx->state = IN_REGEX;
			fputc(c, out);
			ctx->prev_char = c;
			return;
		}
		
		if (c == '{') {
			if (ctx->at_line_start) write_indent(out, ctx);
			fputc(c, out);
			write_newline(out, ctx);
			ctx->indent_level++;
			ctx->prev_char = c;
			return;
		}
		
		if (c == '}') {
			ctx->indent_level--;
			if (ctx->indent_level < 0) ctx->indent_level = 0;
			if (!ctx->at_line_start) write_newline(out, ctx);
			write_indent(out, ctx);
			fputc(c, out);
			write_newline(out, ctx);
			ctx->prev_char = c;
			return;
		}
		
		if (c == ';') {
			if (ctx->at_line_start) write_indent(out, ctx);
			fputc(c, out);
			write_newline(out, ctx);
			ctx->prev_char = c;
			return;
		}
		
		if (isspace(c)) {
			if (c == '\n') {
				if (!ctx->at_line_start) {
					write_newline(out, ctx);
				}
			} else {
				if (!ctx->at_line_start) {
					fputc(' ', out);
				}
			}
			ctx->prev_char = c;
			return;
		}
		
		if (ctx->at_line_start) write_indent(out, ctx);
		fputc(c, out);
		ctx->prev_char = c;
		return;
	}
	
	if (ctx->state == NORMAL) {
		if (c == '<') {
			if (!ctx->at_line_start) write_newline(out, ctx);
			ctx->state = IN_TAG;
			ctx->tag_pos = 0;
			ctx->tag_started = 0;
			ctx->prev_char = c;
			return;
		}
		
		if (isspace(c)) {
			ctx->prev_char = c;
			return;
		}
		
		if (ctx->at_line_start) write_indent(out, ctx);
		fputc(c, out);
		ctx->prev_char = c;
	}
}

void process_file(FILE *in, FILE *out) {
	Context ctx;
	init_context(&ctx);
	
	int c;
	while ((c = fgetc(in)) != EOF) {
		process_char(out, &ctx, c);
	}
	
	if (ctx.lookahead_pos > 0) {
		for (int i = 0; i < ctx.lookahead_pos; i++) {
			fputc(ctx.lookahead[i], out);
		}
	}
	
	if (!ctx.at_line_start) {
		fputc('\n', out);
	}
}

int main(int argc, char **argv) {
	int write_mode = 0;
	int file_start = 1;
	
	if (argc > 1 && strcmp(argv[1], "-w") == 0) {
		write_mode = 1;
		file_start = 2;
	}
	
	if (argc <= file_start) {
		process_file(stdin, stdout);
		return 0;
	}
	
	for (int i = file_start; i < argc; i++) {
		FILE *in = fopen(argv[i], "r");
		if (!in) {
			fprintf(stderr, "Error: cannot open %s\n", argv[i]);
			continue;
		}
		
		if (write_mode) {
			char temp_file[512];
			snprintf(temp_file, sizeof(temp_file), "%s.tmp", argv[i]);
			FILE *out = fopen(temp_file, "w");
			if (!out) {
				fprintf(stderr, "Error: cannot write to %s\n", temp_file);
				fclose(in);
				continue;
			}
			
			process_file(in, out);
			fclose(in);
			fclose(out);
			
			remove(argv[i]);
			rename(temp_file, argv[i]);
		} else {
			process_file(in, stdout);
			fclose(in);
		}
	}
	
	return 0;
}
