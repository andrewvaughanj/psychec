// Copyright (c) 2016/17/18/19/20/21 Leandro T. C. Melo <ltcmelo@gmail.com>
// Copyright (c) 2008 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "Parser__IMPL__.inc"

using namespace psy;
using namespace C;

//-------------//
// Expressions //
//-------------//

/**
 * Parse an \a expression.
 * <a href="https://docs.google.com/spreadsheets/d/1oGjtFaqLzSoBEp2aGNgHrbEHxSi4Ijv57mXMPymZEcQ/edit?usp=sharing">
 * This table
 * </a>
 * describes the choices taken by the parser as according to the grammar rules.
 *
 \verbatim
 expression:
     assignment-expression
     expression , assignment-expression
 \endverbatim
 *
 * \remark 6.5.17
 *
 * \note
 * The naming convention employed in certain expression-parsing methods,
 * e.g., in \c Parser::parseExpressionWithPrecedence_CAST, deviates a bit
 * from the usual convention employed in parsing methods. This difference
 * is due to the precedence-oriented way in which the grammar of
 * expressions is defined; had said method been named \c parseCastExpression,
 * after its rule name \a cast-expression, one could have expected that
 * its result would always be a CastExpressionSyntax node (a correspondence
 * that holds in general). But this is not true, given that a
 * \a cast-expression may actually derive an \a constant, whose node is a
 * ConstantExpressionSyntax.
 */
bool Parser::parseExpression(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();

    DepthControl _(depthOfExprs_);
    return parseExpressionWithPrecedenceComma(expr);
}

/**
 * Parse an \a identifier.
 *
 * \remark 6.4.2 and 6.5.1
 */
bool Parser::parseIdentifierExpression(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();

    if (peek().kind() != SyntaxKind::IdentifierToken) {
        diagnosticsReporter_.ExpectedTokenOfCategoryIdentifier();
        return false;
    }

    parseIdentifierExpression_AtFirst(expr);
    return true;
}

/**
 * Parse an \a identifier as an \a expression, with LA(1) at FIRST.
 *
 * \remark 6.4.2 and 6.5.1
 */
void Parser::parseIdentifierExpression_AtFirst(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == IdentifierToken,
                  return,
                  "assert failure: <identifier>");

    auto identExpr = makeNode<IdentifierExpressionSyntax>();
    expr = identExpr;
    identExpr->identTkIdx_ = consume();
}

/**
 * Parse a \a constant.
 *
 * \remark 6.4.4 and 6.5.1
 */
template <class ExprT>
bool Parser::parseConstant(ExpressionSyntax*& expr, SyntaxKind exprK)
{
    DEBUG_THIS_RULE();

    if (!SyntaxFacts::isConstantToken(peek().kind())) {
        diagnosticsReporter_.ExpectedTokenOfCategoryConstant();
        return false;
    }

    parseConstant_AtFirst<ExprT>(expr, exprK);
    return true;
}

template bool Parser::parseConstant<ConstantExpressionSyntax>
(ExpressionSyntax*& expr, SyntaxKind exprK);

/**
 * Parse a \a constant, with LA(1) at FIRST.
 *
 * \remark 6.4.4 and 6.5.1
 */
template <class ExprT>
void Parser::parseConstant_AtFirst(ExpressionSyntax*& expr, SyntaxKind exprK)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(SyntaxFacts::isConstantToken(peek().kind()),
                  return,
                  "assert failure: <constant>");

    auto constExpr = makeNode<ExprT>(exprK);
    expr = constExpr;
    constExpr->constantTkIdx_  = consume();
}

template void Parser::parseConstant_AtFirst<ConstantExpressionSyntax>
(ExpressionSyntax*& expr, SyntaxKind exprK);

/**
 * Parse a \a string-literal.
 */
bool Parser::parseStringLiteral(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();

    if (!SyntaxFacts::isStringLiteralToken(peek().kind())) {
        diagnosticsReporter_.ExpectedTokenOfCategoryStringLiteral();
        return false;
    }

    parseStringLiteral_AtFirst(expr);
    return true;
}

/**
 * Parse a \a string-literal, with LA(1) at FIRST.
 *
 * \remark 6.4.5 and 6.5.1
 */
void Parser::parseStringLiteral_AtFirst(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(SyntaxFacts::isStringLiteralToken(peek().kind()),
                  return,
                  "assert failure: <string-literal>");

    StringLiteralExpressionSyntax* strLit = nullptr;
    StringLiteralExpressionSyntax** strLit_cur = &strLit;

    do {
        *strLit_cur = makeNode<StringLiteralExpressionSyntax>();
        (*strLit_cur)->litTkIdx_ = consume();
        strLit_cur = &(*strLit_cur)->adjacent_;
    }
    while (SyntaxFacts::isStringLiteralToken(peek().kind()));

    expr = strLit;
}

/**
 * Parse a \a parenthesized-expression, with LA(1) at FIRST.
 *
 * \remark 6.5.1
 */
bool Parser::parseParenthesizedExpression_AtFirst(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == OpenParenToken,
                  return false,
                  "assert failure: `('");

    auto parenExpr = makeNode<ParenthesizedExpressionSyntax>();
    expr = parenExpr;
    parenExpr->openParenTkIdx_ = consume();
    return parseExpression(parenExpr->expr_)
        && matchOrSkipTo(CloseParenToken, &parenExpr->closeParenTkIdx_);
}

/**
 * Parse a GNU extension \a statements-and-declaration in \a expression,
 * with LA(1) at FIRST and LA(2) at FOLLOW.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html#Statement-Exprs
 */
bool Parser::parseExtGNU_StatementExpression_AtFirst(ExpressionSyntax *&expr)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == OpenParenToken
                        && peek(2).kind() == OpenBraceToken,
                  return false,
                  "assert failure: `(' then `{'");

    if (!tree_->options().extensions().isEnabled_ExtGNU_StatementExpressions())
        diagnosticsReporter_.ExpectedFeature("GNU statement expressions");

    auto gnuExpr = makeNode<ExtGNU_EnclosedCompoundStatementExpressionSyntax>();
    expr = gnuExpr;
    gnuExpr->openParenTkIdx_ = consume();

    StatementSyntax* statement = nullptr;
    parseCompoundStatement_AtFirst(statement, StatementContext::None);
    if (statement->asCompoundStatement())
        gnuExpr->stmt_ = statement->asCompoundStatement();
    return matchOrSkipTo(CloseParenToken, &gnuExpr->closeParenTkIdx_);
}

/**
 * Parse a \a primary-expression.
 *
 \verbatim
 primary-expression:
     identifier
     constant
     string-literal
     ( expression )
     generic-selection
 \endverbatim
 *
 * \remark 6.5.1
 */
bool Parser::parsePrimaryExpression(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();

    switch (peek().kind()) {
        case IdentifierToken:
            parseIdentifierExpression_AtFirst(expr);
            break;

        case IntegerConstantToken:
            parseConstant_AtFirst<ConstantExpressionSyntax>(
                    expr,
                    IntegerConstantExpression);
            break;

        case FloatingConstantToken:
            parseConstant_AtFirst<ConstantExpressionSyntax>(
                    expr,
                    FloatingConstantExpression);
            break;

        case CharacterConstantToken:
        case CharacterConstant_L_Token:
        case CharacterConstant_u_Token:
        case CharacterConstant_U_Token:
            parseConstant_AtFirst<ConstantExpressionSyntax>(
                    expr,
                    CharacterConstantExpression);
            break;

        case Keyword_Ext_true:
        case Keyword_Ext_false:
            parseConstant_AtFirst<ConstantExpressionSyntax>(
                    expr,
                    BooleanConstantExpression);
            break;

        case Keyword_Ext_NULL:
        case Keyword_Ext_nullptr:
            parseConstant_AtFirst<ConstantExpressionSyntax>(
                    expr,
                    NULL_ConstantExpression);
            break;

        case StringLiteralToken:
        case StringLiteral_L_Token:
        case StringLiteral_u8_Token:
        case StringLiteral_u_Token:
        case StringLiteral_U_Token:
        case StringLiteral_R_Token:
        case StringLiteral_LR_Token:
        case StringLiteral_u8R_Token:
        case StringLiteral_uR_Token:
        case StringLiteral_UR_Token:
            parseStringLiteral_AtFirst(expr);
            break;

        case OpenParenToken:
            if (peek(2).kind() == OpenBraceToken)
                return parseExtGNU_StatementExpression_AtFirst(expr);
            return parseParenthesizedExpression_AtFirst(expr);

        case Keyword__Generic:
            return parseGenericSelectionExpression_AtFirst(expr);

        default:
            diagnosticsReporter_.ExpectedFIRSTofExpression();
            return false;
    }

    return true;
}

/**
 * Parse a \a generic-selection, with LA(1) at FIRST.
 *
 \verbatim
 generic-selection:
     _Generic ( assignment-expression , generic-assoc-list )
 \endverbatim
 *
 * \remark 6.5.1.1
 */
bool Parser::parseGenericSelectionExpression_AtFirst(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == Keyword__Generic,
                  return false,
                  "assert failure: `_Generic'");

    auto selExpr = makeNode<GenericSelectionExpressionSyntax>();
    expr = selExpr;
    selExpr->genericKwTkIdx_ = consume();

    return match(OpenParenToken, &selExpr->openParenTkIdx_)
            && parseExpressionWithPrecedenceAssignment(selExpr->expr_)
            && match(CommaToken, &selExpr->commaTkIdx_)
            && parseGenericAssociationList(selExpr->assocs_)
            && matchOrSkipTo(CloseParenToken, &selExpr->closeParenTkIdx_);
}

/**
 * Parse a \a generic-assoc-list.
 *
 \verbatim
 generic-assoc-list:
     generic-association
     generic-assoc-list , generic-association
 \endverbatim
 *
 * \remark 6.5.1.1
 */
bool Parser::parseGenericAssociationList(GenericAssociationListSyntax*& assocList)
{
    DEBUG_THIS_RULE();

    return parseCommaSeparatedItems<GenericAssociationSyntax>(
                assocList,
                &Parser::parseGenericAssociation);
}

/**
 * Parse a \a generic-association.
 *
 \verbatim
 generic-association:
     type-name : assignment-expression
     default: assignment-expression
 \endverbatim
 *
 * \remark 6.5.1.1
 */
bool Parser::parseGenericAssociation(GenericAssociationSyntax*& assoc,
                                     GenericAssociationListSyntax*&)
{
    DEBUG_THIS_RULE();

    switch (peek().kind()) {
        case Keyword_default: {
            assoc = makeNode<GenericAssociationSyntax>(DefaultGenericAssociation);
            auto defExpr = makeNode<IdentifierExpressionSyntax>();
            defExpr->identTkIdx_ = consume();
            assoc->typeName_or_default_ = defExpr;
            break;
        }

        default: {
            TypeNameSyntax* typeName = nullptr;
            if (!parseTypeName(typeName))
                return false;
            assoc = makeNode<GenericAssociationSyntax>(TypedGenericAssociation);
            assoc->typeName_or_default_ = typeName;
            break;
        }
    }

    return match(ColonToken, &assoc->colonTkIdx_)
            && parseExpressionWithPrecedenceAssignment(assoc->expr_);
}

/* Postfix */

/**
 * Parse a \a postfix-expression, or any expression that is subsumed by such rule.
 *
 * See note about naming convention in Parser::parseExpression.
 *
 \verbatim
 postfix-expression:
     primary-expression
     postfix-expression [ expression ]
     postfix-expression ( argument-expression-list_opt )
     postfix-expression . identifier
     postfix-expression -> identifier
     postfix-expression ++
     postfix-expression --
     ( type-name ) { initializer-list }
     ( type-name) { initializer-list, }
 \endverbatim
 *
 * Adjusted grammar:
 *
 \verbatim
 postfix-expression:
     primary-expression
     postfix-expression postfix-expression-at-postfix
     compound-literal-at-open-paren
 \endverbatim
 *
 * \remark 6.5.2
 */
bool Parser::parseExpressionWithPrecedencePostfix(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();

    if (peek().kind() == OpenParenToken) {
        switch (peek(2).kind()) {
            // postfix-expression -> `(' type-name ->* type-qualifier ->
            case Keyword_const:
            case Keyword_volatile:
            case Keyword_restrict:
            case Keyword__Atomic:

            // postfix-expression -> `(' type-name ->* alignment-specifier ->
            case Keyword__Alignas:

            // postfix-expression -> `(' type-name ->* GNU-typeof-specifier ->
            case Keyword_ExtGNU___typeof__:

            // postfix-expression -> `(' type-name ->* type-specifier ->
            case Keyword_void:
            case Keyword_char:
            case Keyword_short:
            case Keyword_int:
            case Keyword_long:
            case Keyword_float:
            case Keyword_double:
            case Keyword_signed:
            case Keyword_unsigned:
            case Keyword_Ext_char16_t:
            case Keyword_Ext_char32_t:
            case Keyword_Ext_wchar_t:
            case Keyword__Bool:
            case Keyword__Complex:
            case Keyword_struct:
            case Keyword_union:
            case Keyword_enum:
                return parseCompoundLiteral_AtOpenParen(expr);

            // postfix-expression -> `(' type-name ->* type-specifier -> typedef-name ->
            // postfix-expression -> unary-expression ->* `(' expression ->
            case IdentifierToken: {
                Backtracker BT(this);
                auto openParenTkIdx = consume();
                TypeNameSyntax* typeName = nullptr;
                if (parseTypeName(typeName)
                            && peek().kind() == CloseParenToken
                            && peek(2).kind() == OpenBraceToken) {
                    auto closeParenTkIdx = consume();
                    return parseCompoundLiteral_AtOpenBrace(expr,
                                                            openParenTkIdx,
                                                            typeName,
                                                            closeParenTkIdx);
                }
                BT.backtrack();
                break;
            }

            default:
                break;
        }
    }

    return parsePrimaryExpression(expr)
        && parsePostfixExpression_AtFollow(expr);
}

bool Parser::parsePostfixExpression_AtFollow(ExpressionSyntax*& expr)
{
    while (true) {
        SyntaxKind exprK = UnknownSyntax;
        switch (peek().kind()) {
            /* 6.5.2.1 */
            case OpenBracketToken: {
                if (!parsePostfixExpression_AtPostfix<ArraySubscriptExpressionSyntax>(
                            expr,
                            ElementAccessExpression,
                            [this] (ArraySubscriptExpressionSyntax*& arrExpr) {
                                arrExpr->openBracketTkIdx_ = consume();
                                return parseExpression(arrExpr->arg_)
                                        && matchOrSkipTo(CloseBracketToken, &arrExpr->closeBracketTkIdx_);
                            })) {
                   return false;
                }
                break;
            }

            /* 6.5.2.2 */
            case OpenParenToken: {
                if (!parsePostfixExpression_AtPostfix<CallExpressionSyntax>(
                            expr,
                            CallExpression,
                            [this] (CallExpressionSyntax*& callExpr) {
                                callExpr->openParenTkIdx_ = consume();
                                if (peek().kind() == CloseParenToken) {
                                    callExpr->closeParenTkIdx_ = consume();
                                    return true;
                                }
                                return parseCallArguments(callExpr->args_)
                                        && matchOrSkipTo(CloseParenToken, &callExpr->closeParenTkIdx_);
                            })) {
                    return false;
                }
                break;
            }

            /* 6.5.2.3 */
            case DotToken:
                exprK = DirectMemberAccessExpression;
                [[fallthrough]];

            case ArrowToken: {
                if (exprK == UnknownSyntax)
                    exprK = IndirectMemberAccessExpression;
                if (!parsePostfixExpression_AtPostfix<MemberAccessExpressionSyntax>(
                            expr,
                            exprK,
                            [this] (MemberAccessExpressionSyntax*& membAccess) {
                                membAccess->oprtrTkIdx_ = consume();
                                if (peek().kind() == IdentifierToken) {
                                    ExpressionSyntax* identExpr = nullptr;
                                    parseIdentifierExpression_AtFirst(identExpr);
                                    membAccess->identExpr_ = identExpr->asIdentifierExpression();
                                    return true;
                                }

                                diagnosticsReporter_.ExpectedFieldName();
                                return false;
                            })) {
                    return false;
                }
                break;
            }

            /* 6.5.2.4 */
            case PlusPlusToken:
                exprK = PostIncrementExpression;
                [[fallthrough]];

            case MinusMinusToken: {
                if (exprK == UnknownSyntax)
                    exprK = PostDecrementExpression;
                if (!parsePostfixExpression_AtPostfix<PostfixUnaryExpressionSyntax>(
                            expr,
                            exprK,
                            [this] (PostfixUnaryExpressionSyntax*& incDecExpr) {
                                incDecExpr->oprtrTkIdx_ = consume();
                                return true;
                            })) {
                    return false;
                }
                break;
            }

            default:
                return true;
        }
    }
}

/**
 * Parse a \a postfix-expression, with LA(1) at the postfix start.
 *
 * In the adjusted grammar of Parser::parseExpressionWithPrecedencePostfix.
 *
 \verbatim
 postfix-expression-at-postfix:
     [ expression ]
     ( argument-expression-list_opt )
     . identifier
     -> identifier
     ++
     --
 \endverbatim
 */
template <class ExprT>
bool Parser::parsePostfixExpression_AtPostfix(ExpressionSyntax*& expr,
                                              SyntaxKind exprK,
                                              std::function<bool(ExprT*&)> parsePostfix)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == OpenBracketToken
                        || peek().kind() == OpenParenToken
                        || peek().kind() == DotToken
                        || peek().kind() == ArrowToken
                        || peek().kind() == PlusPlusToken
                        || peek().kind() == MinusMinusToken,
                  return false,
                  "assert failure: `[', `(', `.', `->', '++', or `--'");

    auto postfixExpr = makeNode<ExprT>(exprK);
    postfixExpr->expr_ = expr;
    expr = postfixExpr;
    return parsePostfix(postfixExpr);
}

/**
 * Parse an \a argument-expression-list.
 *
 \verbatim
 argument-expression-list:
     assignment-expression
     argument-expression-list , assignment-expression
 \endverbatim
 *
 * \remark 6.5.2
 */
bool Parser::parseCallArguments(ExpressionListSyntax*& exprList)
{
    DEBUG_THIS_RULE();

    return parseCommaSeparatedItems<ExpressionSyntax>(
                exprList,
                &Parser::parseCallArgument);
}

bool Parser::parseCallArgument(ExpressionSyntax*&expr, ExpressionListSyntax*&)
{
    return parseExpressionWithPrecedenceAssignment(expr);
}

/**
 * Parse a \a postfix-expression that is a compound literal,
 * with LA(1) at \c (.
 *
 * In the adjusted grammar of Parser::parseExpressionWithPrecedencePostfix.
 *
 \verbatim
 compound-literal-at-open-paren:
    ( type-name ) compound-literal-at-open-brace
 \endverbatim
 */
bool Parser::parseCompoundLiteral_AtOpenParen(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == OpenParenToken,
                  return false,
                  "assert failure: `('");

    auto openParenTkIdx = consume();
    TypeNameSyntax* typeName = nullptr;
    if (!parseTypeName(typeName))
        return false;

    auto closeParenTkIdx = LexedTokens::invalidIndex();
    if (!match(CloseParenToken, &closeParenTkIdx))
        return false;

    if (peek().kind() != OpenBraceToken) {
        diagnosticsReporter_.ExpectedToken(OpenBraceToken);
        return false;
    }

    return parseCompoundLiteral_AtOpenBrace(expr, openParenTkIdx, typeName, closeParenTkIdx);
}

/**
 * Parse a \a postfix-expression that is a compound literal,
 * with LA(1) at \c {.
 *
 * In the adjusted grammar of Parser::parseCompoundLiteral_AtOpenParen.
 *
 \verbatim
 compound-literal-at-open-brace:
    { initializer-list }
    { initializer-list, }
 \endverbatim
 */

bool Parser::parseCompoundLiteral_AtOpenBrace(
        ExpressionSyntax*& expr,
        LexedTokens::IndexType openParenTkIdx,
        TypeNameSyntax* typeName,
        LexedTokens::IndexType closeParenTkIdx)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == OpenBraceToken,
                  return false,
                  "assert failure: `{'");

    if (tree_->dialect().std() < LanguageDialect::Std::C99
            && !tree_->options().extensions().isEnabled_ExtGNU_CompoundLiterals())
        diagnosticsReporter_.ExpectedFeature("GNU/C99 compound literals");

    auto compLit = makeNode<CompoundLiteralExpressionSyntax>();
    expr = compLit;
    compLit->openParenTkIdx_ = openParenTkIdx;
    compLit->typeName_ = typeName;
    compLit->closeParenTkIdx_ = closeParenTkIdx;
    return parseInitializer(compLit->init_)
        && parsePostfixExpression_AtFollow(expr);
}

/* Unary */

/**
 * Parse a \a unary-expression, or any expression that is subsumed by such rule.
 *
 * See note about naming convention in Parser::parseExpression.
 *
 \verbatim
 unary-expression:
     postfix-expression
     ++ unary-expression
     -- unary-expression
     unary-operator cast-expression
     sizeof unary-expression
     sizeof ( type-name )
     _Alignof ( type-name )

 unary-operator: & * + - ~ !
 \endverbatim
 *
 * Adjusted grammar:
 *
 \verbatim
 unary-expression:
     postfix-expression
     prefix-unary-expression-at-first unary-expression
     prefix-unary-expression-at-first cast-expression
     type-trait-expression
 \endverbatim
 *
 * \remark 6.5.3
 */
bool Parser::parseExpressionWithPrecedenceUnary(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();

    switch (peek().kind()) {
        /* 6.5.3.1 */
        case PlusPlusToken:
            return parsePrefixUnaryExpression_AtFirst(
                        expr,
                        PreIncrementExpression,
                        &Parser::parseExpressionWithPrecedenceUnary);

        case MinusMinusToken:
            return parsePrefixUnaryExpression_AtFirst(
                        expr,
                        PreDecrementExpression,
                        &Parser::parseExpressionWithPrecedenceUnary);

        /* 6.5.3.2 */
        case AmpersandToken:
            return parsePrefixUnaryExpression_AtFirst(
                        expr,
                        AddressOfExpression,
                        &Parser::parseExpressionWithPrecedenceCast);

        case AsteriskToken:
            return parsePrefixUnaryExpression_AtFirst(
                        expr,
                        PointerIndirectionExpression,
                        &Parser::parseExpressionWithPrecedenceCast);

        /* 6.5.3.3 */
        case PlusToken:
            return parsePrefixUnaryExpression_AtFirst(
                        expr,
                        UnaryPlusExpression,
                        &Parser::parseExpressionWithPrecedenceCast);

        case MinusToken:
            return parsePrefixUnaryExpression_AtFirst(
                        expr,
                        UnaryMinusExpression,
                        &Parser::parseExpressionWithPrecedenceCast);

        case TildeToken:
            return parsePrefixUnaryExpression_AtFirst(
                        expr,
                        BitwiseNotExpression,
                        &Parser::parseExpressionWithPrecedenceCast);

        case ExclamationToken:
            return parsePrefixUnaryExpression_AtFirst(
                        expr,
                        LogicalNotExpression,
                        &Parser::parseExpressionWithPrecedenceCast);

        /* 6.5.3.4 */
        case Keyword_sizeof:
            return parseTypeTraitExpression_AtFirst(expr, SizeofExpression);

        case Keyword__Alignof:
            return parseTypeTraitExpression_AtFirst(expr, AlignofExpression);

        default:
            return parseExpressionWithPrecedencePostfix(expr);
    }
}

/**
 * Parse a \a unary-expression that is a \b prefix \a unary expression,
 * with LA(1) at the operator.
 *
 * In the adjusted grammar of Parser::parseExpressionWithPrecedenceUnary.
 *
 \verbatim
 prefix-unary-expression-at-first:
     ++ unary-expression
     -- unary-expression
     unary-operator cast-expression

 unary-operator: & * + - ~ !
 \endverbatim
 *
 * \remark 6.5.3
 */
bool Parser::parsePrefixUnaryExpression_AtFirst(
        ExpressionSyntax*& expr,
        SyntaxKind exprK,
        bool (Parser::*parseOperand)(ExpressionSyntax*&))
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == PlusPlusToken
                    || peek().kind() == MinusMinusToken
                    || peek().kind() == AmpersandToken
                    || peek().kind() == AsteriskToken
                    || peek().kind() == PlusToken
                    || peek().kind() == MinusToken
                    || peek().kind() == TildeToken
                    || peek().kind() == ExclamationToken,
                  return false,
                  "expected `[', `(', `.', `->', '++', or `--'");

    auto unaryExpr = makeNode<PrefixUnaryExpressionSyntax>(exprK);
    expr = unaryExpr;
    unaryExpr->oprtrTkIdx_ = consume();
    return ((this)->*parseOperand)(unaryExpr->expr_);
}

/**
 * Parse a \a unary-expression that is type-trait \a unary expression,
 * with LA(1) at FIRST.
 *
 * In the adjusted grammar of Parser::parseExpressionWithPrecedenceUnary.
 *
 \verbatim
 type-trait-expression:
     sizeof unary-expression
     sizeof ( type-name )
     _Alignof ( type-name )
 \endverbatim
 *
 * \remark 6.5.3
 */
bool Parser::parseTypeTraitExpression_AtFirst(ExpressionSyntax*& expr, SyntaxKind exprK)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == Keyword_sizeof
                    || peek().kind() == Keyword__Alignof,
                  return false,
                  "assert failure: `sizeof' or `_Alignof'");

    auto traitExpr = makeNode<TypeTraitExpressionSyntax>(exprK);
    expr = traitExpr;
    traitExpr->oprtrTkIdx_ = consume();

    return parseParenthesizedTypeNameOrExpression(traitExpr->tyRef_);
}

/* Cast */

/**
 * Parse a \a cast-expression, or any expression that is subsumed by such rule.
 *
 * See note about naming convention in Parser::parseExpression.
 *
 \verbatim
 cast-expression:
     unary-expression
     ( type-name ) cast-expression
 \endverbatim
 *
 * \remark 6.5.4
 */
bool Parser::parseExpressionWithPrecedenceCast(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();

    switch (peek().kind()) {
        case OpenParenToken: {
            switch (peek(2).kind()) {
                // cast-expression -> `(' type-name ->* type-qualifier ->
                case Keyword_const:
                case Keyword_volatile:
                case Keyword_restrict:
                case Keyword__Atomic:

                // cast-expression -> `(' type-name ->* alignment-specifier ->
                case Keyword__Alignas:

                // cast-expression -> `(' type-name ->* GNU-typeof-specifier ->
                case Keyword_ExtGNU___typeof__:

                // cast-expression -> `(' type-name ->* type-specifier ->
                case Keyword_void:
                case Keyword_char:
                case Keyword_short:
                case Keyword_int:
                case Keyword_long:
                case Keyword_float:
                case Keyword_double:
                case Keyword_signed:
                case Keyword_unsigned:
                case Keyword_Ext_char16_t:
                case Keyword_Ext_char32_t:
                case Keyword_Ext_wchar_t:
                case Keyword__Bool:
                case Keyword__Complex:
                case Keyword_struct:
                case Keyword_union:
                case Keyword_enum:
                    return parseCompoundLiteralOrCastExpression_AtFirst(expr);

                // cast-expression -> `(' type-name ->* type-specifier -> typedef-name ->
                // cast-expression -> unary-expression ->* `(' expression ->
                case IdentifierToken: {
                    Backtracker BT(this);
                    if (parseCompoundLiteralOrCastExpression_AtFirst(expr)) {
                        if (expr->kind() == CastExpression)
                            maybeAmbiguateCastExpression(expr);
                        return true;
                    }
                    BT.backtrack();
                    return parseExpressionWithPrecedenceUnary(expr);
                }

                default:
                    [[fallthrough]];
            }
        }

        case PlusPlusToken:
        case MinusMinusToken:
        case AmpersandToken:
        case AsteriskToken:
        case PlusToken:
        case MinusToken:
        case TildeToken:
        case ExclamationToken:
        case Keyword_sizeof:
        case Keyword__Alignof:
        case IdentifierToken:
        case IntegerConstantToken:
        case FloatingConstantToken:
        case CharacterConstantToken:
        case CharacterConstant_L_Token:
        case CharacterConstant_u_Token:
        case CharacterConstant_U_Token:
        case Keyword_Ext_true:
        case Keyword_Ext_false:
        case Keyword_Ext_NULL:
        case Keyword_Ext_nullptr:
        case StringLiteralToken:
        case StringLiteral_L_Token:
        case StringLiteral_u8_Token:
        case StringLiteral_u_Token:
        case StringLiteral_U_Token:
        case StringLiteral_R_Token:
        case StringLiteral_LR_Token:
        case StringLiteral_u8R_Token:
        case StringLiteral_uR_Token:
        case StringLiteral_UR_Token:
        case Keyword__Generic:
            return parseExpressionWithPrecedenceUnary(expr);

        case Keyword_ExtGNU___extension__: {
            auto extKwTkIdx = consume();
            if (!parseExpressionWithPrecedenceCast(expr))
                return false;
            PSYCHE_ASSERT(expr, return false, "invalid expression");
            expr->extKwTkIdx_ = extKwTkIdx;
            return true;
        }


        default:
            diagnosticsReporter_.ExpectedFIRSTofExpression();
            return false;
    }
}

bool Parser::parseCompoundLiteralOrCastExpression_AtFirst(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == OpenParenToken,
                  return false,
                  "assert failure: `('");

    auto openParenTkIdx = consume();
    TypeNameSyntax* typeName = nullptr;
    if (!parseTypeName(typeName))
        return false;

    LexedTokens::IndexType closeParenTkIdx;
    if (!match(CloseParenToken, &closeParenTkIdx))
        return false;

    if (peek().kind() == OpenBraceToken)
        return parseCompoundLiteral_AtOpenBrace(expr,
                                                openParenTkIdx,
                                                typeName,
                                                closeParenTkIdx);

    auto castExpr = makeNode<CastExpressionSyntax>();
    expr = castExpr;
    castExpr->openParenTkIdx_ = openParenTkIdx;
    castExpr->typeName_ = typeName;
    castExpr->closeParenTkIdx_ = closeParenTkIdx;
    return parseExpressionWithPrecedenceCast(castExpr->expr_);
}

void Parser::maybeAmbiguateCastExpression(ExpressionSyntax*& expr)
{
    PSYCHE_ASSERT(expr->kind() == CastExpression,
                  return, "");

    auto castExpr = expr->asCastExpression();
    auto prefixExpr = castExpr->expr_->asPrefixUnaryExpression();
    if (!(prefixExpr->asPrefixUnaryExpression()
            && (prefixExpr->kind() == AddressOfExpression
                    || prefixExpr->kind() == PointerIndirectionExpression
                    || prefixExpr->kind() == UnaryPlusExpression
                    || prefixExpr->kind() == UnaryMinusExpression)))
        return;

    TypeNameSyntax* typeName = castExpr->typeName_;
    if (!(typeName->specs_
            && typeName->specs_->value->kind() == TypedefName
            && !typeName->specs_->next
            && typeName->decltor_
            && typeName->decltor_->kind() == AbstractDeclarator))
        return;

    SyntaxKind binExprK;
    switch (prefixExpr->kind()) {
        case AddressOfExpression:
            binExprK = BitwiseANDExpression;
            break;

        case PointerIndirectionExpression:
            binExprK = MultiplyExpression;
            break;

        case UnaryPlusExpression:
            binExprK = AddExpression;
            break;

        case UnaryMinusExpression:
            binExprK = SubstractExpression;
            break;

        default:
            PSYCHE_ASSERT(false, return, "");
    }

    auto nameExpr = makeNode<IdentifierExpressionSyntax>();
    nameExpr->identTkIdx_ =
            typeName->specs_->value->asTypedefName()->identTkIdx_;
    auto parenExpr = makeNode<ParenthesizedExpressionSyntax>();
    parenExpr->expr_ = nameExpr;
    parenExpr->openParenTkIdx_ = castExpr->openParenTkIdx_;
    auto binExpr = makeNode<BinaryExpressionSyntax>(binExprK);
    binExpr->leftExpr_ = parenExpr;
    parenExpr->closeParenTkIdx_ = castExpr->closeParenTkIdx_;
    binExpr->oprtrTkIdx_ = prefixExpr->oprtrTkIdx_;
    binExpr->rightExpr_ = prefixExpr->expr_;

    auto ambiExpr = makeNode<AmbiguousCastOrBinaryExpressionSyntax>(AmbiguousCastOrBinaryExpression);
    expr = ambiExpr;
    ambiExpr->castExpr_ = castExpr;
    ambiExpr->binExpr_ = binExpr;
}

/* N-ary */

namespace NAryPrecedence {

enum : std::uint8_t
{
    Undefined = 0,
    Sequencing,
    Assignment,
    Conditional,
    LogicalOR,
    LogicalAND,
    BitwiseOR,
    BitwiseXOR,
    BitwiseAND,
    Equality,
    Relational,
    Shift,
    Additive,
    Multiplicative
};

} // NAryPrecedence

std::uint8_t precedenceOf(SyntaxKind tkK)
{
    switch (tkK) {
        case CommaToken:
            return NAryPrecedence::Sequencing;

        case EqualsToken:
        case PlusEqualsToken:
        case MinusEqualsToken:
        case AsteriskEqualsToken:
        case SlashEqualsToken:
        case PercentEqualsToken:
        case LessThanLessThanEqualsToken:
        case GreaterThanGreaterThanEqualsToken:
        case AmpersandEqualsToken:
        case CaretEqualsToken:
        case BarEqualsToken:
            return NAryPrecedence::Assignment;

        case QuestionToken:
            return NAryPrecedence::Conditional;

        case BarBarToken:
            return NAryPrecedence::LogicalOR;

        case AmpersandAmpersandToken:
            return NAryPrecedence::LogicalAND;

        case BarToken:
            return NAryPrecedence::BitwiseOR;

        case CaretToken:
            return NAryPrecedence::BitwiseXOR;

        case AmpersandToken:
            return NAryPrecedence::BitwiseAND;

        case EqualsEqualsToken:
        case ExclamationEqualsToken:
            return NAryPrecedence::Equality;

        case GreaterThanToken:
        case LessThanToken:
        case LessThanEqualsToken:
        case GreaterThanEqualsToken:
            return NAryPrecedence::Relational;

        case LessThanLessThanToken:
        case GreaterThanGreaterThanToken:
            return NAryPrecedence::Shift;

        case PlusToken:
        case MinusToken:
            return NAryPrecedence::Additive;

        case AsteriskToken:
        case SlashToken:
        case PercentToken:
            return NAryPrecedence::Multiplicative;

        default:
            return NAryPrecedence::Undefined;
    }
}

bool isRightAssociative(SyntaxKind tkK)
{
    auto prec = precedenceOf(tkK);
    return prec == NAryPrecedence::Conditional || prec == NAryPrecedence::Assignment;
}

/**
 * Parse a \a multiplicative-expression, or any expression that is subsumed by such rule.
 *
 * See note about naming convention in Parser::parseExpression.
 *
 \verbatim
 multiplicative-expression:
     cast-expression
     multiplicative-expression * cast-expression
     multiplicative-expression / cast-expression
     multiplicative-expression % cast-expression
 \endverbatim
 *
 * \remark 6.5.5
 */
bool Parser::parseExpressionWithPrecedenceMultiplicative(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();

    return parseNAryExpression(expr, NAryPrecedence::Multiplicative);
}

/**
 * Parse a \a additive-expression, or any expression that is subsumed by such rule.
 *
 * See note about naming convention in Parser::parseExpression.
 *
 \verbatim
 additive-expression:
     multiplicative-expression
     additive-expression + multiplicative-expression
     additive-expression - multiplicative-expression
 \endverbatim
 *
 * \remark 6.5.6
 */
bool Parser::parseExpressionWithPrecedenceAdditive(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();

    return parseNAryExpression(expr, NAryPrecedence::Additive);
}

/**
 * Parse a \a shift-expression, or any expression that is subsumed by such rule.
 *
 * See note about naming convention in Parser::parseExpression.
 *
 \verbatim
 shift-expression:
     additive-expression
     shift-expression << additive-expression
     shift-expression >> additive-expression
 \endverbatim
 *
 * \remark 6.5.7
 */
bool Parser::parseExpressionWithPrecedenceShift(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();

    return parseNAryExpression(expr, NAryPrecedence::Shift);
}

/**
 * Parse a \a relational-expression, or any expression that is subsumed by such rule.
 *
 * See note about naming convention in Parser::parseExpression.
 *
 \verbatim
 relational-expression:
     shift-expression
     relational-expression < shift-expression
     relational-expression > shift-expression
     relational-expression <= shift-expression
     relational-expression >= shift-expression
 \endverbatim
 *
 * \remark 6.5.8
 */
bool Parser::parseExpressionWithPrecedenceRelational(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();

    return parseNAryExpression(expr, NAryPrecedence::Relational);
}

/**
 * Parse a \a multiplicative-expression, or any expression that is subsumed by such rule.
 *
 * See note about naming convention in Parser::parseExpression.
 *
 \verbatim
 equality-expression:
     relational-expression
     equality-expression == relational-expression
     equality-expression != relational-expression
 \endverbatim
 *
 * \remark 6.5.9
 */
bool Parser::parseExpressionWithPrecedenceEquality(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();

    return parseNAryExpression(expr, NAryPrecedence::Equality);
}

/**
 * Parse a \a AND-expression, or any expression that is subsumed by such rule.
 *
 * See note about naming convention in Parser::parseExpression.
 *
 \verbatim
 AND-expression:
     equality-expression
     AND-expression & equality-expression
 \endverbatim
 *
 * \remark 6.5.10
 */
bool Parser::parseExpressionWithPrecedenceBitwiseAND(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();

    return parseNAryExpression(expr, NAryPrecedence::BitwiseAND);
}

/**
 * Parse a \a exclusive-OR-expression, or any expression that is subsumed by such rule.
 *
 * See note about naming convention in Parser::parseExpression.
 *
 \verbatim
 exclusive-OR-expression:
     AND-expression
     exclusive-OR-expression ^ AND-expression
 \endverbatim
 *
 * \remark 6.5.11
 */
bool Parser::parseExpressionWithPrecedenceBitwiseXOR(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();

    return parseNAryExpression(expr, NAryPrecedence::BitwiseXOR);
}

/**
 * Parse a \a inclusive-OR-expression, or any expression that is subsumed by such rule.
 *
 * See note about naming convention in Parser::parseExpression.
 *
 \verbatim
 inclusive-OR-expression:
     exclusive-OR-expression
     inclusive-OR-expression | exclusive-OR-expression
 \endverbatim
 *
 * \remark 6.5.12
 */
bool Parser::parseExpressionWithPrecedenceBitwiseOR(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();

    return parseNAryExpression(expr, NAryPrecedence::BitwiseOR);
}

/**
 * Parse a \a logical-AND-expression, or any expression that is subsumed by such rule.
 *
 * See note about naming convention in Parser::parseExpression.
 *
 \verbatim
 logical-AND-expression:
     inclusive-OR-expression
     logical-AND-expression && inclusive-OR-expression
 \endverbatim
 *
 * \remark 6.5.13
 */
bool Parser::parseExpressionWithPrecedenceLogicalAND(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();

    return parseNAryExpression(expr, NAryPrecedence::LogicalAND);
}

/**
 * Parse a \a logical-OR-expression, or any expression that is subsumed by such rule.
 *
 * See note about naming convention in Parser::parseExpression.
 *
 \verbatim
 logical-OR-expression:
     logical-AND-expression
     logical-OR-expression || logical-AND-expression
 \endverbatim
 *
 * \remark 6.5.14
 */
bool Parser::parseExpressionWithPrecedenceLogicalOR(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();

    return parseNAryExpression(expr, NAryPrecedence::LogicalOR);
}

/**
 * Parse a \a conditional-expression, or any expression that is subsumed by such rule.
 *
 * See note about naming convention in Parser::parseExpression.
 *
 \verbatim
 conditional-expression:
     logical-OR-expression
     logical-OR-expression ? expression : conditional-expression
 \endverbatim
 *
 * \remark 6.5.15
 */
bool Parser::parseExpressionWithPrecedenceConditional(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();

    return parseNAryExpression(expr, NAryPrecedence::Conditional);
}

/**
 * Parse a \a assignment-expression, or any expression that is subsumed by such rule.
 *
 * See note about naming convention in Parser::parseExpression.
 *
 \verbatim
 assignment-expression:
     conditional-expression
     unary-expression assignment-operator assignment-expression

 assignment-operator: one of
     = *= /= %= += -= <<= >>= &= ^= |=
 \endverbatim
 *
 * \remark 6.5.16
 */
bool Parser::parseExpressionWithPrecedenceAssignment(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();

    return parseNAryExpression(expr, NAryPrecedence::Assignment);
}

/**
 * Parse the comman operator, which is a sequence of expressions.
 *
 * \remark 6.5.17
 * \remark Parser::parseExpression
 */
bool Parser::parseExpressionWithPrecedenceComma(ExpressionSyntax*& expr)
{
    DEBUG_THIS_RULE();

    return parseNAryExpression(expr, NAryPrecedence::Sequencing);
}

bool Parser::parseNAryExpression(ExpressionSyntax*& expr, std::uint8_t cutoffPrecedence)
{
    DEBUG_THIS_RULE();

    if (!parseExpressionWithPrecedenceCast(expr))
        return false;

    return parseNAryExpression_AtOperator(expr, cutoffPrecedence);
}

bool Parser::parseNAryExpression_AtOperator(ExpressionSyntax*& baseExpr,
                                            std::uint8_t cutoffPrecedence)
{
    DEBUG_THIS_RULE();

    auto curExprDepth = depthOfExprs_;

    while (precedenceOf(peek().kind()) >= cutoffPrecedence) {
        if (++curExprDepth > MAX_DEPTH_OF_EXPRS)
            throw std::runtime_error("maximum depth of expressions reached");

        auto curTkK = peek().kind();
        auto exprK = SyntaxFacts::NAryExpressionKind(curTkK);
        auto oprtrTkIdx = consume();

        ConditionalExpressionSyntax* condExpr = nullptr;
        if (curTkK == QuestionToken) {
            condExpr = makeNode<ConditionalExpressionSyntax>();
            condExpr->questionTkIdx_ = oprtrTkIdx;

            if (peek().kind() == ColonToken) {
                if (!tree_->options().extensions().isEnabled_ExtGNU_StatementExpressions())
                    diagnosticsReporter_.ExpectedFeature("GNU conditionals");

                condExpr->whenTrueExpr_ = nullptr;
            }
            else {
                parseExpression(condExpr->whenTrueExpr_);
            }
            match(ColonToken, &condExpr->colonTkIdx_);
        }

        ExpressionSyntax* nextExpr = nullptr;
        if (!parseExpressionWithPrecedenceCast(nextExpr))
             return false;

        auto refPrec = precedenceOf(curTkK);
        curTkK = peek().kind();
        auto nextPrec = precedenceOf(curTkK);
        while ((nextPrec > refPrec
                        && SyntaxFacts::isNAryOperatorToken(curTkK))
                   || (nextPrec == refPrec
                        && isRightAssociative(curTkK))) {
            if (!parseNAryExpression_AtOperator(nextExpr, nextPrec))
                return false;

            curTkK = peek().kind();
            nextPrec = precedenceOf(curTkK);
        }

        if (condExpr) {
            condExpr->condExpr_ = baseExpr;
            condExpr->whenFalseExpr_ = nextExpr;
            baseExpr = condExpr;
        }
        else {
            if (SyntaxFacts::isAssignmentExpression(exprK)) {
                baseExpr = fill_LeftOperandInfixOperatorRightOperand_MIXIN(
                                makeNode<AssignmentExpressionSyntax>(exprK),
                                baseExpr, oprtrTkIdx, nextExpr);
            }
            else if (SyntaxFacts::isBinaryExpression(exprK)) {
                baseExpr = fill_LeftOperandInfixOperatorRightOperand_MIXIN(
                                makeNode<BinaryExpressionSyntax>(exprK),
                                baseExpr, oprtrTkIdx, nextExpr);
            }
            else {
                baseExpr = fill_LeftOperandInfixOperatorRightOperand_MIXIN(
                                makeNode<SequencingExpressionSyntax>(),
                                baseExpr, oprtrTkIdx, nextExpr);
            }
        }
    }

    return true;
}

template <class NodeT>
NodeT* Parser::fill_LeftOperandInfixOperatorRightOperand_MIXIN(
        NodeT* expr,
        ExpressionSyntax* left,
        LexedTokens::IndexType opTkIdx,
        ExpressionSyntax* right)
{
    expr->leftExpr_ = left;
    expr->oprtrTkIdx_ = opTkIdx;
    expr->rightExpr_ = right;
    return expr;
}
